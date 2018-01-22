/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * Jeroen Domburg <jeroen@spritesmods.com> wrote this file. As long as you retain
 * this notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return.
 * ----------------------------------------------------------------------------
 */

/*
This is example code for the esphttpd library. It's a small-ish demo showing off
the server, including WiFi connection management capabilities, some IO etc.
*/

extern "C" {
#include <libesphttpd/esp.h>
#include "libesphttpd/httpd.h"
#include "io.h"
#include "libesphttpd/httpdespfs.h"
#include "cgi.h"
#include "libesphttpd/cgiwifi.h"
#include "libesphttpd/cgiflash.h"
#include "libesphttpd/auth.h"
#include "libesphttpd/espfs.h"
#include "libesphttpd/captdns.h"
#include "libesphttpd/webpages-espfs.h"
#include "libesphttpd/cgiwebsocket.h"
#include "cgi-test.h"
}
#include <iostream>
#include "templates.hpp"
#include "esp_wifi.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#ifdef ESP32
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"
#include "tcpip_adapter.h"


#endif

#include "config.hpp"

#define TAG "user_main"

//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
int myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no==0) {
		strcpy(user, "admin");
		strcpy(pass, "s3cr3t");
		return 1;
//Add more users this way. Check against incrementing no for each user added.
//	} else if (no==1) {
//		strcpy(user, "user1");
//		strcpy(pass, "something");
//		return 1;
	}
	return 0;
}


//Broadcast the uptime in seconds every second over connected websockets
static void websocketBcast(void *arg) {
	static int ctr=0;
	char buff[128];
	while(1) {
		ctr++;
		sprintf(buff, "Up for %d minutes %d seconds!\n", ctr/60, ctr%60);
		cgiWebsockBroadcast("/websocket/ws.cgi", buff, strlen(buff), WEBSOCK_FLAG_NONE);
		vTaskDelay(1000/portTICK_RATE_MS);
	}
}

//On reception of a message, send "You sent: " plus whatever the other side sent
static void myWebsocketRecv(Websock *ws, char *data, int len, int flags) {
	int i;
	char buff[128];
	sprintf(buff, "You sent: ");
	for (i=0; i<len; i++) buff[i+10]=data[i];
	buff[i+10]=0;
	cgiWebsocketSend(ws, buff, strlen(buff), WEBSOCK_FLAG_NONE);
}

//Websocket connected. Install reception handler and send welcome message.
static void myWebsocketConnect(Websock *ws) {
	ws->recvCb=myWebsocketRecv;
	cgiWebsocketSend(ws, "Hi, Websocket!", 14, WEBSOCK_FLAG_NONE);
}

//On reception of a message, echo it back verbatim
void myEchoWebsocketRecv(Websock *ws, char *data, int len, int flags) {
	printf("EchoWs: echo, len=%d\n", len);
	cgiWebsocketSend(ws, data, len, flags);
}

//Echo websocket connected. Install reception handler.
void myEchoWebsocketConnect(Websock *ws) {
	printf("EchoWs: connect\n");
	ws->recvCb=myEchoWebsocketRecv;
}

#define OTA_FLASH_SIZE_K 1024
#define OTA_TAGNAME "generic"

CgiUploadFlashDef uploadParams={
	.type=CGIFLASH_TYPE_FW,
	.fw1Pos=0x1000,
	.fw2Pos=((OTA_FLASH_SIZE_K*1024)/2)+0x1000,
	.fwSize=((OTA_FLASH_SIZE_K*1024)/2)-0x1000,
	.tagName=OTA_TAGNAME
};


/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
HttpdBuiltInUrl builtInUrls[]={
	ROUTE_REDIRECT("/", "/index.html"),
	//ROUTE_CGI("/config.json", cgiGetConfigJson),
#if 0
	ROUTE_CGI_ARG("*", cgiRedirectApClientToHostname, "esp8266.nonet"),
	ROUTE_REDIRECT("/", "/index.tpl"),

	ROUTE_TPL("/led.tpl", tplLed),
	ROUTE_TPL("/index.tpl", tplCounter),
	ROUTE_CGI("/led.cgi", cgiLed),

	ROUTE_REDIRECT("/flash", "/flash/index.html"),
	ROUTE_REDIRECT("/flash/", "/flash/index.html"),
	ROUTE_CGI_ARG("/flash/next", cgiGetFirmwareNext, &uploadParams),
	ROUTE_CGI_ARG("/flash/upload", cgiUploadFirmware, &uploadParams),
	ROUTE_CGI("/flash/reboot", cgiRebootFirmware),

	//Routines to make the /wifi URL and everything beneath it work.
//Enable the line below to protect the WiFi configuration with an username/password combo.
//	{"/wifi/*", authBasic, myPassFn},

	ROUTE_REDIRECT("/wifi", "/wifi/wifi.tpl"),
	ROUTE_REDIRECT("/wifi/", "/wifi/wifi.tpl"),
	ROUTE_CGI("/wifi/wifiscan.cgi", cgiWiFiScan),
	ROUTE_TPL("/wifi/wifi.tpl", tplWlan),
	ROUTE_CGI("/wifi/connect.cgi", cgiWiFiConnect),
	ROUTE_CGI("/wifi/connstatus.cgi", cgiWiFiConnStatus),
	ROUTE_CGI("/wifi/setmode.cgi", cgiWiFiSetMode),

	ROUTE_REDIRECT("/websocket", "/websocket/index.html"),
	ROUTE_WS("/websocket/ws.cgi", myWebsocketConnect),
	ROUTE_WS("/websocket/echo.cgi", myEchoWebsocketConnect),

	ROUTE_REDIRECT("/test", "/test/index.html"),
	ROUTE_REDIRECT("/test", "/test/index.html"),
	ROUTE_CGI("/test/test.cgi", cgiTestbed),
#endif
	ROUTE_FILESYSTEM(),

	ROUTE_END()
};


#ifdef ESP32

static EventGroupHandle_t wifi_ap_event_group;
static EventGroupHandle_t wifi_sta_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const static int CONNECTED_BIT = BIT0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_sta_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_sta_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_START:
    {
        tcpip_adapter_ip_info_t ap_ip_info;
        if (tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_AP, &ap_ip_info) == 0) {
            ESP_LOGI(TAG, "~~~~~~~~~~~");
            ESP_LOGI(TAG, "IP:" IPSTR, IP2STR(&ap_ip_info.ip));
            ESP_LOGI(TAG, "MASK:" IPSTR, IP2STR(&ap_ip_info.netmask));
            ESP_LOGI(TAG, "GW:" IPSTR, IP2STR(&ap_ip_info.gw));
            ESP_LOGI(TAG, "~~~~~~~~~~~");
        }
    }
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR" join,AID=%d\n",
                MAC2STR(event->event_info.sta_connected.mac),
                event->event_info.sta_connected.aid);
        xEventGroupSetBits(wifi_ap_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR"leave,AID=%d\n",
                MAC2STR(event->event_info.sta_disconnected.mac),
                event->event_info.sta_disconnected.aid);
        xEventGroupClearBits(wifi_ap_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_SCAN_DONE:
        break;
    default:
        break;
    }
    return ESP_OK;
}


//Simple task to connect to an access point
void ICACHE_FLASH_ATTR init_wifi(bool modeAP) {
	nvs_flash_init();

	wifi_sta_event_group = xEventGroupCreate();
	wifi_ap_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK( esp_event_loop_init(wifi_event_handler, NULL) );

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
        ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

	//Go to station mode
	esp_wifi_disconnect();

	if(modeAP) {
		ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_AP) );

		wifi_config_t ap_config;
		strcpy((char*)(&ap_config.ap.ssid), "ESP");
		ap_config.ap.ssid_len = 3;
		ap_config.ap.channel = 1;
		ap_config.ap.authmode = WIFI_AUTH_OPEN;
		ap_config.ap.ssid_hidden = 0;
		ap_config.ap.max_connection = 1;
		ap_config.ap.beacon_interval = 100;

		esp_wifi_set_config(WIFI_IF_AP, &ap_config);
	}
	else {
		esp_wifi_set_mode(WIFI_MODE_STA);

		//Connect to the defined access point.
		wifi_config_t config;
		memset(&config, 0, sizeof(config));
		sprintf((char*)config.sta.ssid, "everest36");			// @TODO: Changeme
		sprintf((char*)config.sta.password, "mb90f497"); 	// @TODO: Changeme
		esp_wifi_set_config(WIFI_IF_STA, &config);
		esp_wifi_connect();
	}

	ESP_ERROR_CHECK( esp_wifi_start() );
}
#endif

using namespace ecuspy;

const ConfigEntry Cfg3[] = {
		ConfigEntry{"id1", "name1", "desc1", cfgCatWIFI, cfgTypeString, 10},
		ConfigEntry{"id2", "name2Str", "desc2", cfgCatWIFI, cfgTypeString, 15},
		NumEntry<uint8_t>{"id3", "name3Int", "desc3", cfgCatWIFI, cfgTypeInt8, 3, 0, 200},
		NumEntry<double>{"id4", "name4Double", "desc4", cfgCatWIFI, cfgTypeDouble, 10, 0.0, 10.1},
		CustomValidatorEntry{"id5", "name5Double", "desc5", cfgCatWIFI, 15,
			[](const ConfigEntry& e, const char* str) {return !strcmp(str, "kuku");}}};

//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.

void CfgTest() {
	try {
	Config::instance().initialize(Cfg3, tpl::countof(Cfg3));
	Config::instance().setValueStr(0, "abcd");
	Config::instance().setValueStr(1, "1234");
	int rc = getConfig<int>("id2");
	std::cout << "RC=" << rc << std::endl;
	{
		Transaction tr;
		Config::instance().setValueStr(0, "efgh");
		Config::instance().setValueStr(1, "4321");
		rc = getConfig<int>("id2");
		std::cout << "RC2=" << rc << std::endl;
	}
	rc = getConfig<int>("id2");
	std::cout << "RC3=" << rc << std::endl;
	} catch(std::exception& e) {
		std::cout << "Settings set exception" << std::endl;
	}
}

extern "C" void app_main(void) {

	try {
		throw int(20);
	} catch(int e) {
		std::cout << "Caught " << e << std::endl;
	}

	ioInit();

	CfgTest();
	//LocalConfig.get<int>(0);

	espFsInit((void*)(webpages_espfs_start));

	tcpip_adapter_init();
	httpdInit(builtInUrls, 80, HTTPD_FLAG_NONE);

	init_wifi(false); // Supply false for STA mode

	xTaskCreate(websocketBcast, "wsbcast", 3000, NULL, 3, NULL);

	printf("\nReady\n");
}
