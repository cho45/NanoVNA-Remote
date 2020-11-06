/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "driver/uart.h"
#include "esp_ota_ops.h"

static const char *TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

#define HOST_NAME "nanovna.test"
#define MAX_OPEN_SOCKETS 13

typedef struct rest_server_context {
	char base_path[ESP_VFS_PATH_MAX + 1];
	char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;


struct async_resp_arg {
	httpd_handle_t hd;
	int fd;
	uint8_t data[128];
	size_t data_len;
};

static httpd_handle_t server;

void ws_broadcast(uint8_t* data, size_t len)
{
	if (!server) return;
	if (!len) return;
	size_t clients = MAX_OPEN_SOCKETS;
	int    client_fds[clients];
	if (httpd_get_client_list(server, &clients, client_fds) == ESP_OK) {
		for (size_t i=0; i < clients; ++i) {
			int sock = client_fds[i];
			if (httpd_ws_get_fd_info(server, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
				ESP_LOGI(TAG, "client (fd=%d) -> sending async len=%d", sock, len);
				httpd_ws_frame_t ws_pkt;
				memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
				ws_pkt.payload = data;
				ws_pkt.len = len;
				ws_pkt.type = HTTPD_WS_TYPE_BINARY;

				httpd_ws_send_frame_async(server, sock, &ws_pkt);
			}
		}
	} else {
		ESP_LOGE(TAG, "httpd_get_client_list failed!");
		return;
	}
}

static esp_err_t ws_handler(httpd_req_t *req)
{
	uint8_t buf[128] = { 0 };

	httpd_ws_frame_t ws_pkt;
	memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

	ws_pkt.payload = buf;
	esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 126);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
		return ret;
	}
	ESP_LOGI(TAG, "ws packet %d:%d", ws_pkt.type, ws_pkt.len);

	/*
	// echo test
	ret = httpd_ws_send_frame(req, &ws_pkt);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
	}
	*/

	buf[ws_pkt.len] = 0;
	// ESP_LOGI(TAG, "uart write bytes %d %s", ws_pkt.len, buf);
	ESP_LOGI(TAG, "uart write bytes %d", ws_pkt.len);
	uart_write_bytes(UART_NUM_2, (const char *) ws_pkt.payload, ws_pkt.len);

	return ret;
}

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
	const char *type = "text/plain";
	if (CHECK_FILE_EXTENSION(filepath, ".html")) {
		type = "text/html";
	} else
	if (CHECK_FILE_EXTENSION(filepath, ".wasm")) {
		type = "application/wasm";
	} else
	if (CHECK_FILE_EXTENSION(filepath, ".js")) {
		type = "application/javascript";
	} else
	if (CHECK_FILE_EXTENSION(filepath, ".mjs")) {
		type = "application/javascript";
	} else
	if (CHECK_FILE_EXTENSION(filepath, ".css")) {
		type = "text/css";
	} else
	if (CHECK_FILE_EXTENSION(filepath, ".png")) {
		type = "image/png";
	} else
	if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
		type = "image/x-icon";
	} else
	if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
		type = "text/xml";
	}
	return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
	char filepath[FILE_PATH_MAX];
	bool is_gzip = 0;

	// use filepath for temporary buffer
	esp_err_t err = httpd_req_get_hdr_value_str(req, "Host", filepath, sizeof(filepath));
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get Host header");
		return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Failed to get Host header");
	}

	if (
		strncmp(filepath, HOST_NAME, strlen(HOST_NAME)) != 0 &&
		strncmp(filepath, "192.", strlen("192.")) != 0
	) {
		ESP_LOGE(TAG, "Host %s is not matched to ours. redirect", filepath);
		httpd_resp_set_status(req, "303 See Other");
		httpd_resp_set_type(req, HTTPD_TYPE_TEXT);
		httpd_resp_set_hdr(req, "Location", "http://" HOST_NAME "/#ws");
		return httpd_resp_send(req, "see other", HTTPD_RESP_USE_STRLEN);
	}

	rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
	strlcpy(filepath, rest_context->base_path, sizeof(filepath));
	if (req->uri[strlen(req->uri) - 1] == '/') {
		strlcat(filepath, "/index.html", sizeof(filepath));
	} else {
		strlcat(filepath, req->uri, sizeof(filepath));
	}

	// first, search .gz file name
	strlcat(filepath, ".gz", sizeof(filepath));

	int fd = open(filepath, O_RDONLY, 0);
	filepath[strlen(filepath)-3] = 0; // restore original path
	if (fd == -1) {
		int fd = open(filepath, O_RDONLY, 0);

		if (fd == -1) {
			ESP_LOGE(TAG, "Failed to open file : %s", filepath);
			httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Failed to read existing file");
			return ESP_FAIL;
		}
	} else {
		is_gzip = 1;
	}

	set_content_type_from_file(req, filepath);
	if (is_gzip) {
		httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
	}

	ESP_LOGI(TAG, "File sending %s", filepath);

	char *chunk = rest_context->scratch;
	ssize_t read_bytes;
	do {
		/* Read file in chunks into the scratch buffer */
		read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
		if (read_bytes == -1) {
			ESP_LOGE(TAG, "Failed to read file : %s", filepath);
		} else if (read_bytes > 0) {
			/* Send the buffer contents as HTTP response chunk */
			if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
				close(fd);
				ESP_LOGE(TAG, "File sending failed!");
				/* Abort sending file */
				httpd_resp_sendstr_chunk(req, NULL);
				/* Respond with 500 Internal Server Error */
				httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
				return ESP_FAIL;
			}
		}
	} while (read_bytes > 0);
	/* Close file after sending complete */
	close(fd);
	ESP_LOGI(TAG, "File sending complete %s", filepath);
	/* Respond with an empty chunk to signal HTTP response completion */
	return httpd_resp_send_chunk(req, NULL, 0);
}

///* Simple handler for light brightness control */
//static esp_err_t light_brightness_post_handler(httpd_req_t *req)
//{
//	int total_len = req->content_len;
//	int cur_len = 0;
//	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
//	int received = 0;
//	if (total_len >= SCRATCH_BUFSIZE) {
//		/* Respond with 500 Internal Server Error */
//		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
//		return ESP_FAIL;
//	}
//	while (cur_len < total_len) {
//		received = httpd_req_recv(req, buf + cur_len, total_len);
//		if (received <= 0) {
//			/* Respond with 500 Internal Server Error */
//			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
//			return ESP_FAIL;
//		}
//		cur_len += received;
//	}
//	buf[total_len] = '\0';
//
//	cJSON *root = cJSON_Parse(buf);
//	int red = cJSON_GetObjectItem(root, "red")->valueint;
//	int green = cJSON_GetObjectItem(root, "green")->valueint;
//	int blue = cJSON_GetObjectItem(root, "blue")->valueint;
//	ESP_LOGI(TAG, "Light control: red = %d, green = %d, blue = %d", red, green, blue);
//	cJSON_Delete(root);
//	httpd_resp_sendstr(req, "Post control value successfully");
//	return ESP_OK;
//}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
	httpd_resp_set_type(req, "application/json");

	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);

	const esp_app_desc_t* app_desc = esp_ota_get_app_description();

	cJSON *root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "idf_version", IDF_VER);
	cJSON_AddStringToObject(root, "app_version", app_desc->version);
	cJSON_AddStringToObject(root, "compile_date", app_desc->date);
	cJSON_AddStringToObject(root, "compile_time", app_desc->time);
	cJSON_AddNumberToObject(root, "cores", chip_info.cores);
	cJSON_AddNumberToObject(root, "minimum_free_heap_size", esp_get_minimum_free_heap_size());
	cJSON_AddNumberToObject(root, "free_heap_size", esp_get_free_heap_size());

	switch (esp_reset_reason()) {
		case ESP_RST_UNKNOWN: cJSON_AddStringToObject(root, "reset_reason", "UNKNOWN"); break;
		case ESP_RST_POWERON: cJSON_AddStringToObject(root, "reset_reason", "POWERON"); break;
		case ESP_RST_EXT: cJSON_AddStringToObject(root, "reset_reason", "EXT"); break;
		case ESP_RST_SW: cJSON_AddStringToObject(root, "reset_reason", "SW"); break;
		case ESP_RST_PANIC: cJSON_AddStringToObject(root, "reset_reason", "PANIC"); break;
		case ESP_RST_INT_WDT: cJSON_AddStringToObject(root, "reset_reason", "INT_WDT"); break;
		case ESP_RST_TASK_WDT: cJSON_AddStringToObject(root, "reset_reason", "TASK_WDT"); break;
		case ESP_RST_WDT: cJSON_AddStringToObject(root, "reset_reason", "WDT"); break;
		case ESP_RST_DEEPSLEEP: cJSON_AddStringToObject(root, "reset_reason", "DEEPSLEEP"); break;
		case ESP_RST_BROWNOUT: cJSON_AddStringToObject(root, "reset_reason", "BROWNOUT"); break;
		case ESP_RST_SDIO: cJSON_AddStringToObject(root, "reset_reason", "SDIO"); break;
	}

	const char *sys_info = cJSON_Print(root);
	httpd_resp_sendstr(req, sys_info);
	free((void *)sys_info);
	cJSON_Delete(root);
	return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
	REST_CHECK(base_path, "wrong base path", err);
	rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
	REST_CHECK(rest_context, "No memory for rest context", err);
	strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

	httpd_config_t config = {
		.task_priority      = tskIDLE_PRIORITY+5,
		.stack_size         = 4096,
		.core_id            = tskNO_AFFINITY,
		.server_port        = 80,
		.ctrl_port          = 32768,
		.max_open_sockets   = MAX_OPEN_SOCKETS,
		.max_uri_handlers   = 8,
		.max_resp_headers   = 8,
		.backlog_conn       = 5,
		.lru_purge_enable   = true,
		.recv_wait_timeout  = 5,
		.send_wait_timeout  = 5,
		.global_user_ctx = NULL,
		.global_user_ctx_free_fn = NULL,
		.global_transport_ctx = NULL,
		.global_transport_ctx_free_fn = NULL,
		.open_fn = NULL,
		.close_fn = NULL,
		.uri_match_fn = NULL
	};

	config.uri_match_fn = httpd_uri_match_wildcard;

	ESP_LOGI(TAG, "Starting HTTP Server");
	REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

	/* URI handler for fetching system info */
	httpd_uri_t system_info_get_uri = {
		.uri = "/api/v1/system/info",
		.method = HTTP_GET,
		.handler = system_info_get_handler,
		.user_ctx = rest_context
	};
	httpd_register_uri_handler(server, &system_info_get_uri);

//	/* URI handler for light brightness control */
//	httpd_uri_t light_brightness_post_uri = {
//		.uri = "/api/v1/light/brightness",
//		.method = HTTP_POST,
//		.handler = light_brightness_post_handler,
//		.user_ctx = rest_context
//	};
//	httpd_register_uri_handler(server, &light_brightness_post_uri);

	httpd_uri_t ws_uri = {
		.uri = "/ws",
		.method = HTTP_GET,
		.handler = ws_handler,
		.user_ctx = rest_context,
		.is_websocket = true,
	};
	httpd_register_uri_handler(server, &ws_uri);

	/* URI handler for getting web server files */
	httpd_uri_t common_get_uri = {
		.uri = "/*",
		.method = HTTP_GET,
		.handler = rest_common_get_handler,
		.user_ctx = rest_context
	};
	httpd_register_uri_handler(server, &common_get_uri);

	return ESP_OK;
err_start:
	free(rest_context);
err:
	return ESP_FAIL;
}
