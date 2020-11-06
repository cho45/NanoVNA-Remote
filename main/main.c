#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "protocol_examples_common.h"

#define MDNS_INSTANCE "esp home web server"
#define EXAMPLE_ESP_WIFI_SSID      "NanoVNA"
#define EXAMPLE_ESP_WIFI_PASS      "madakimetenai"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       1

#define WWW_MOUNT_POINT "/www"

esp_netif_t *netif_ap = NULL;

static const char *TAG = "main";

void ws_broadcast(uint8_t* data, size_t len);
esp_err_t start_rest_server(const char *base_path);
esp_err_t start_dns_server();

static void initialise_mdns(void) {
	mdns_init();
	mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
	mdns_instance_name_set(MDNS_INSTANCE);

	mdns_txt_item_t serviceTxtData[] = {
		{"board", "esp32"},
		{"path", "/"}
	};

	ESP_ERROR_CHECK(mdns_service_add("NanoVNA-WebServer", "_http", "_tcp", 80, serviceTxtData,
				sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

static esp_err_t set_dhcps_dns(esp_netif_t *netif, uint32_t addr) {
	esp_netif_dns_info_t dns;
	dns.ip.u_addr.ip4.addr = addr;
	dns.ip.type = IPADDR_TYPE_V4;
	dhcps_offer_t dhcps_dns_value = OFFER_DNS;
	ESP_ERROR_CHECK(esp_netif_dhcps_option(netif, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_dns_value, sizeof(dhcps_dns_value)));
	ESP_ERROR_CHECK(esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns));
	return ESP_OK;
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
	if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
		ESP_LOGI(TAG, "station "MACSTR" join, AID=%d", MAC2STR(event->mac), event->aid);
	} else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
		wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
		ESP_LOGI(TAG, "station "MACSTR" leave, AID=%d", MAC2STR(event->mac), event->aid);
	}
}

void wifi_init_softap(void) {
	netif_ap = esp_netif_create_default_wifi_ap();
	esp_netif_ip_info_t ip_info;
	esp_netif_get_ip_info(netif_ap, &ip_info);
	uint32_t sip = ip_info.ip.addr;

	set_dhcps_dns(netif_ap, sip);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(
		WIFI_EVENT,
		ESP_EVENT_ANY_ID,
		&wifi_event_handler,
		NULL,
		NULL
	));

	wifi_config_t wifi_config = {
		.ap = {
			.ssid = EXAMPLE_ESP_WIFI_SSID,
			.ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
			.channel = EXAMPLE_ESP_WIFI_CHANNEL,
			.password = EXAMPLE_ESP_WIFI_PASS,
			.max_connection = EXAMPLE_MAX_STA_CONN,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};
	if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
			EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);


	ESP_LOGI(TAG, "sip=%d.%d.%d.%d", sip&0xFF, (sip>>8)&0xFF, (sip>>16)&0xFF, (sip>>24)&0xFF);
}

esp_err_t init_fs(void) {
	esp_vfs_spiffs_conf_t conf = {
		.base_path = WWW_MOUNT_POINT,
		.partition_label = NULL,
		.max_files = 5,
		.format_if_mount_failed = false
	};
	esp_err_t ret = esp_vfs_spiffs_register(&conf);

	if (ret != ESP_OK) {
		if (ret == ESP_FAIL) {
			ESP_LOGE(TAG, "Failed to mount or format filesystem");
		} else if (ret == ESP_ERR_NOT_FOUND) {
			ESP_LOGE(TAG, "Failed to find SPIFFS partition");
		} else {
			ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return ESP_FAIL;
	}

	size_t total = 0, used = 0;
	ret = esp_spiffs_info(NULL, &total, &used);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
	} else {
		ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	}
	return ESP_OK;
}

static void uart_rx_task(void *arg) {
	uart_config_t uart_config = {
		// .baud_rate = 115200,
		// .baud_rate = 460800,
		.baud_rate = 921600,
		.data_bits = UART_DATA_8_BITS,
		.parity    = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
		.flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
		.source_clk = UART_SCLK_APB,
	};
	int intr_alloc_flags = 0;

	const size_t BUF_SIZE = 512;

	int rx_buffer_size = BUF_SIZE * 2;
	int tx_buffer_size = 0;
	int queue_size = 0;
	QueueHandle_t* queue_handle = NULL;
	ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, rx_buffer_size, tx_buffer_size, queue_size, queue_handle, intr_alloc_flags));
	ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
	// Set UART pins(TX: IO16, RX: IO17, RTS: IO18, CTS: IO19)
	ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 16, 17, 18, 19));

	uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

	TickType_t prevTick = xTaskGetTickCount();
	size_t total_len = 0;

	while (1) {
		// Read data from the UART
		int len = uart_read_bytes(UART_NUM_2, data + total_len, BUF_SIZE - total_len, 20 / portTICK_RATE_MS);
		if (len > 0) {
			ESP_LOGI(TAG, "uart read bytes %d", len);
			// ESP_LOGI(TAG, "uart read bytes %d %s", len, data);
			total_len += len;
		}

		if (total_len == 0) {
			continue;
		}

		if (
			total_len >= BUF_SIZE || (
				(xTaskGetTickCount() - prevTick) * portTICK_RATE_MS > 20
			)
		) {
			// Write data back to the UART
			// uart_write_bytes(UART_NUM_2, (const char *) data, len);
			data[total_len] = 0;
			ESP_LOGI(TAG, "ws write %d %s", total_len, data);
			ws_broadcast(data, total_len);
			total_len = 0;
		}
	}
}


void app_main(void) {
	gpio_config_t io_conf;
	io_conf.pin_bit_mask = (1ULL<<GPIO_NUM_15);
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	io_conf.pull_down_en = 0;
	io_conf.intr_type = GPIO_INTR_DISABLE;
	gpio_config(&io_conf);

	ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	initialise_mdns();
	netbiosns_init();
	netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);


	if (gpio_get_level(GPIO_NUM_15)) {
		// open
		ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
		wifi_init_softap();
	} else {
		// connect to gnd
		ESP_ERROR_CHECK(example_connect());
	}

	const size_t UART_TASK_STACK_SIZE = 2048;
	xTaskCreate(uart_rx_task, "uart_rx_task", UART_TASK_STACK_SIZE, NULL, 10, NULL);

	ESP_ERROR_CHECK(init_fs());
	ESP_ERROR_CHECK(start_rest_server(WWW_MOUNT_POINT));
	ESP_ERROR_CHECK(start_dns_server());
}
