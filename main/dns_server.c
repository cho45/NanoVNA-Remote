
#include <stdint.h>
#include <string.h>

#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#define DNS_QR_QUERY 0
#define DNS_QR_RESPONSE 1

#define DNS_OPCODE_QUERY 0
#define DNS_OPCODE_IQUERY 1
#define DNS_OPCODE_STATUS 2

#define DNS_FLAGS_RCODE(flags) ((flags>>0)&0b1111)
#define DNS_FLAGS_CD(flags) ((flags>>4)&0b1)
#define DNS_FLAGS_AD(flags) ((flags>>5)&0b1)
#define DNS_FLAGS_Z(flags) ((flags>>6)&0b1)
#define DNS_FLAGS_RA(flags) ((flags>>7)&0b1)
#define DNS_FLAGS_RD(flags) ((flags>>8)&0b1)
#define DNS_FLAGS_TC(flags) ((flags>>9)&0b1)
#define DNS_FLAGS_AA(flags) ((flags>>10)&0b1)
#define DNS_FLAGS_OPCODE(flags) ((flags>>11)&0b1111)
#define DNS_FLAGS_QR(flags) ((flags>>15)&0b1)

#define DNS_FLAGS_RA (1<<7)
#define DNS_FLAGS_RD (1<<8)
#define DNS_FLAGS_QR_QUERY (0<<15)
#define DNS_FLAGS_QR_RESPONSE (1<<15)


#define DNS_RCODE_NO_ERROR 0
#define DNS_RCODE_FORMAT_ERROR 1
#define DNS_RCODE_SERVER_FAILURE 2
#define DNS_RCODE_NAME_ERROR 3
#define DNS_RCODE_NOT_IMPLEMENTED 4
#define DNS_RCODE_REFUSED 5

extern esp_netif_t *netif_ap;

static const char *TAG = "dns";

#define DNS_HEADER_SIZE 12
typedef struct {
	uint16_t ID;
	uint16_t FLAGS;
	uint16_t QDCOUNT;
	uint16_t ANCOUNT;
	uint16_t NSCOUNT;
	uint16_t ARCOUNT;
} __attribute__((packed)) dns_header_t;

typedef struct {
	uint32_t TTL;
	uint16_t RDLENGTH;
	uint32_t ADDR;
} __attribute__((packed)) dns_resource_a_t;


int dns_parse_header(const uint8_t* data, dns_header_t* header) {
	memcpy(header, data, DNS_HEADER_SIZE);

	header->ID = ntohs(header->ID);
	header->QDCOUNT = ntohs(header->QDCOUNT);
	header->ANCOUNT = ntohs(header->ANCOUNT);
	header->NSCOUNT = ntohs(header->NSCOUNT);
	header->ARCOUNT = ntohs(header->ARCOUNT);

	return 1;
}


int dns_write_header(uint8_t* data, const dns_header_t* _header) {
	dns_header_t* header = (dns_header_t*)data;

	header->ID = htons(_header->ID);
	header->FLAGS = htons(_header->FLAGS);
	header->QDCOUNT = htons(_header->QDCOUNT);
	header->ANCOUNT = htons(_header->ANCOUNT);
	header->NSCOUNT = htons(_header->NSCOUNT);
	header->ARCOUNT = htons(_header->ARCOUNT);

	return 1;
}

int dns_write_resource_a(uint8_t* data, const dns_resource_a_t* _resource) {
	dns_resource_a_t* resource = (dns_resource_a_t*)data;

	resource->TTL = htonl(_resource->TTL);
	resource->RDLENGTH = htons(_resource->RDLENGTH);
	resource->ADDR = _resource->ADDR;

	return 1;
}

static void udp_server_task() {
	uint8_t rx_buffer[512];
	char addr_str[128];
	int ip_protocol = 0;
	struct sockaddr_in6 dest_addr;

	const in_port_t PORT = 53;

	while (1) {
		struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
		dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
		dest_addr_ip4->sin_family = AF_INET;
		dest_addr_ip4->sin_port = htons(PORT);
		ip_protocol = IPPROTO_IP;

		int sock = socket(AF_INET, SOCK_DGRAM, ip_protocol);
		if (sock < 0) {
			ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
			break;
		}
		ESP_LOGI(TAG, "Socket created");

		int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (err < 0) {
			ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
		}
		ESP_LOGI(TAG, "Socket bound, port %d", PORT);

		while (1) {
			ESP_LOGI(TAG, "Waiting for data");
			struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
			socklen_t socklen = sizeof(source_addr);
			int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

			// Error occurred during receiving
			if (len < 0) {
				ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
				break;
			}

			// Data received
			else {
				// Get the sender's ip address as string
				if (source_addr.sin6_family == PF_INET) {
					inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
				} else if (source_addr.sin6_family == PF_INET6) {
					inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
				}

				ESP_LOGI(TAG, "Received %d bytes from %s", len, addr_str);

				dns_header_t header;
				dns_parse_header(rx_buffer, &header);

				if (DNS_FLAGS_QR(header.FLAGS) == DNS_QR_QUERY && header.QDCOUNT == 1) {
					// question section
					uint8_t* domain = rx_buffer + DNS_HEADER_SIZE;
					size_t domain_len = strnlen((char*)domain, sizeof(rx_buffer) - DNS_HEADER_SIZE) + 1;
					/*
					uint16_t type  = *(domain + domain_len);
					uint16_t class = *(domain + domain_len + 2);
					*/

					esp_netif_ip_info_t ip_info;
					esp_netif_get_ip_info(netif_ap, &ip_info);
					uint32_t sip = ip_info.ip.addr;

					header.FLAGS = DNS_FLAGS_QR_RESPONSE | DNS_FLAGS_RA | DNS_FLAGS_RD;
					header.QDCOUNT = 1;
					header.ANCOUNT = 1;
					header.NSCOUNT = 0;
					header.ARCOUNT = 0;
					dns_write_header(rx_buffer, &header);
					len = sizeof(header);
					// domain_len + type + class
					len += domain_len + 2 + 2;

					// copy query
					memcpy(rx_buffer + len, domain, domain_len + 2 + 2);
					len += domain_len + 2 + 2;

					dns_resource_a_t resource;
					resource.TTL = 300;
					resource.RDLENGTH = 4;
					resource.ADDR = sip;
					dns_write_resource_a(rx_buffer + len, &resource);
					len += sizeof(resource);

					ESP_LOGI(TAG, "Send response %d bytes to %s", len, addr_str);
					int err = sendto(sock, rx_buffer, len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
					if (err < 0) {
						ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
						break;
					}
				} else {
					ESP_LOGE(TAG, "Unexpected dns packet ID:%04x FLAGS:%04x QDCOUNT:%d ANCOUNT:%d NSCOUNT:%d ARCOUNT:%d",
							header.ID,
							header.FLAGS,
							header.QDCOUNT,
							header.ANCOUNT,
							header.NSCOUNT,
							header.ARCOUNT
							);
				}

			}
		}

		if (sock != -1) {
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			shutdown(sock, 0);
			close(sock);
		}
	}
	vTaskDelete(NULL);
}

esp_err_t start_dns_server() {
	xTaskCreate(udp_server_task, "dns_server", 4096, NULL, 5, NULL);
	return ESP_OK;
}
