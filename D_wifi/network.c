#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cmsis_os2.h"
#include "ohos_init.h"

#include "wifi_device.h"
#include "wifi_hotspot.h"
#include "wifi_error_code.h" 
#include "lwip/netifapi.h"
#include "lwip/api_shell.h"
#include "lwip/ip4_addr.h"
#include "lwip/netif.h"

#define AP_SSID "BearPi"
#define AP_PSK  "0987654321"

#define TASK_STACK_SIZE (1024 * 10)
#define TASK_DELAY_2S 200
#define ONE_SECOND 1
#define DEF_TIMEOUT 15
#define CHANNEL_NUM 7
#define MAC_ADDRESS_LEN 32
#define MAC_DATA0 0
#define MAC_DATA1 1
#define MAC_DATA2 2
#define MAC_DATA3 3
#define MAC_DATA4 4
#define MAC_DATA5 5
#define UDP_SERVERPORT 8888
#define SELECT_WLAN_PORT "wlan0"

#define SELECT_WIFI_SSID "yuizhu"
#define SELECT_WIFI_PASSWORD "zhangran20040906"
#define SELECT_WIFI_SECURITYTYPE WIFI_SEC_TYPE_PSK
#define DHCP_DELAY 100

static struct netif *g_lwip_netif = NULL;
static int g_apEnableSuccess = 0;
static WifiEvent g_wifiEventHandler = { 0 };
WifiErrorCode error;
//

//
static int WiFiInit(void);
static void WaitSacnResult(void);
static int WaitConnectResult(void);
static void OnWifiScanStateChangedHandler(int state, int size);
static void OnWifiConnectionChangedHandler(int state, WifiLinkedInfo *info);
static void OnHotspotStaJoinHandler(StationInfo *info);
static void OnHotspotStateChangedHandler(int state);
static void OnHotspotStaLeaveHandler(StationInfo *info);
static void OnHotspotStaJoinHandler(StationInfo *info);
static void OnHotspotStateChangedHandler(int state);
static void OnHotspotStaLeaveHandler(StationInfo *info);
static int g_staScanSuccess = 0;
static int g_connectSuccess = 0;
static int g_ssid_count = 0;


static void StartUdpServer(void)  //设置UDP服务器
{
    /****************以下为UDP服务器代码,默认IP:192.168.5.1***************/
    // 在sock_fd 进行监听
    int sock_fd;
    // 服务端地址信息
    struct sockaddr_in server_sock;

    // 创建socket
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket is error.\r\n");
        return -1;
    }

    bzero(&server_sock, sizeof(server_sock));
    server_sock.sin_family = AF_INET;
    server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
    server_sock.sin_port = htons(UDP_SERVERPORT);

    // 调用bind函数绑定socket和地址
    if (bind(sock_fd, (struct sockaddr *)&server_sock, sizeof(struct sockaddr)) == -1) {
        perror("bind is error.\r\n");
        return -1;
    }

    int ret;
    char recvBuf[512] = { 0 };
    // 客户端地址信息
    struct sockaddr_in client_addr;
    int size_client_addr = sizeof(struct sockaddr_in);
    while (1) {
        printf("Waiting to receive data...\r\n");
        memset_s(recvBuf, sizeof(recvBuf), 0, sizeof(recvBuf));
        ret = recvfrom(sock_fd, recvBuf, sizeof(recvBuf), 0, (struct sockaddr*)&client_addr,
            (socklen_t*)&size_client_addr);
        if (ret < 0) {
            printf("UDP server receive failed!\r\n");
            return -1;
        }
        printf("receive %d bytes of data from ipaddr = %s, port = %d.\r\n", ret, inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port));
        printf("data is %s\r\n", recvBuf);
        ret = sendto(sock_fd, recvBuf, strlen(recvBuf), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        if (ret < 0) {
            printf("UDP server send failed!\r\n");
            return -1;
        }
    }
    /*********************END********************/
}

static int WifiConnectAp(const char *ssid, const char *psk, WifiScanInfo *info, int i)
{
    if (strcmp(ssid, info[i].ssid) == 0) {
        int result;
        printf("Select:%3d wireless, Waiting...\r\n", i + 1);

        // 拷贝要连接的热点信息
        WifiDeviceConfig select_ap_config = { 0 };
        strcpy_s(select_ap_config.ssid, sizeof(select_ap_config.ssid), info[i].ssid);
        strcpy_s(select_ap_config.preSharedKey, sizeof(select_ap_config.preSharedKey), psk);
        select_ap_config.securityType = WIFI_SEC_TYPE_PSK;

        if (AddDeviceConfig(&select_ap_config, &result) == WIFI_SUCCESS) {
            if (ConnectTo(result) == WIFI_SUCCESS && WaitConnectResult() == 1) {
                g_lwip_netif = netifapi_netif_find(SELECT_WLAN_PORT);
                return 0;
            }
        }
    }
    return -1;
}

static BOOL WifiStaTask(void)
{
    // unsigned int size = WIFI_SCAN_HOTSPOT_LIMIT;

    // // 初始化WIFI
    // if (WiFiInit() != WIFI_SUCCESS) {
    //     printf("WiFiInit failed, error = %d\r\n", error);
    //     return -1;
    // }
    // // 分配空间，保存WiFi信息
    // WifiScanInfo *info = malloc(sizeof(WifiScanInfo) * WIFI_SCAN_HOTSPOT_LIMIT);
    // if (info == NULL) {
    //     return -1;
    // }
    // // 轮询查找WiFi列表
    // do {
    //     Scan();
    //     WaitSacnResult();
    //     error = GetScanInfoList(info, &size);
    // } while (g_staScanSuccess != 1);
    // // 打印WiFi列表
    // printf("********************\r\n");
    // for (uint8_t i = 0; i < g_ssid_count; i++) {
    //     printf("no:%03d, ssid:%-30s, rssi:%5d\r\n", i + 1, info[i].ssid, info[i].rssi);
    // }
    // printf("********************\r\n");
    // // 连接指定的WiFi热点
    // for (uint8_t i = 0; i < g_ssid_count; i++) {
    //     if (WifiConnectAp(SELECT_WIFI_SSID, SELECT_WIFI_PASSWORD, info, i) == WIFI_SUCCESS) {
    //         printf("WiFi connect succeed!\r\n");
    //         break;
    //     }

    //     if (i == g_ssid_count - 1) {
    //         printf("ERROR: No wifi as expected\r\n");
    //         while (1)
    //             osDelay(DHCP_DELAY);
    //     }
    // }

    // // 启动DHCP
    // if (g_lwip_netif) {
    //     dhcp_start(g_lwip_netif);
    //     printf("begain to dhcp\r\n");
    // }
    // // 等待DHCP
    // for (;;) {
    //     if (dhcp_is_bound(g_lwip_netif) == ERR_OK) {
    //         printf("<-- DHCP state:OK -->\r\n");
    //         // 打印获取到的IP信息
    //         netifapi_netif_common(g_lwip_netif, dhcp_clients_info_show, NULL);
    //         break;
    //     }
    //     osDelay(DHCP_DELAY);
    // }

    // 执行其他操作
    // 延时2S便于查看日志
    osDelay(TASK_DELAY_2S);

    // 注册wifi事件的回调函数
    g_wifiEventHandler.OnHotspotStaJoin = OnHotspotStaJoinHandler;
    g_wifiEventHandler.OnHotspotStaLeave = OnHotspotStaLeaveHandler;
    g_wifiEventHandler.OnHotspotStateChanged = OnHotspotStateChangedHandler;
    error = RegisterWifiEvent(&g_wifiEventHandler);
    if (error != WIFI_SUCCESS) {
        printf("RegisterWifiEvent failed, error = %d.\r\n", error);
        return -1;
    }
    printf("RegisterWifiEvent succeed!\r\n");
    // 检查热点模式是否使能
    if (IsHotspotActive() == WIFI_HOTSPOT_ACTIVE) {
        printf("Wifi station is  actived.\r\n");
        return -1;
    }
    // 设置指定的热点配置
    HotspotConfig config = { 0 };

    strcpy_s(config.ssid, strlen(AP_SSID) + 1, AP_SSID);
    strcpy_s(config.preSharedKey, strlen(AP_PSK) + 1, AP_PSK);
    config.securityType = WIFI_SEC_TYPE_PSK;
    config.band = HOTSPOT_BAND_TYPE_2G;
    config.channelNum = CHANNEL_NUM;

    error = SetHotspotConfig(&config);
    if (error != WIFI_SUCCESS) {
        printf("SetHotspotConfig failed, error = %d.\r\n", error);
        return -1;
    }
    printf("SetHotspotConfig succeed!\r\n");

    // 启动wifi热点模式
    error = EnableHotspot();
    if (error != WIFI_SUCCESS) {
        printf("EnableHotspot failed, error = %d.\r\n", error);
        return -1;
    }
    printf("EnableHotspot succeed!\r\n");

    StartUdpServer();

    unsigned int size = WIFI_SCAN_HOTSPOT_LIMIT;

    // 初始化WIFI
    if (WiFiInit() != WIFI_SUCCESS) {
        printf("WiFiInit failed, error = %d\r\n", error);
        return -1;
    }
    // 分配空间，保存WiFi信息
    WifiScanInfo *info = malloc(sizeof(WifiScanInfo) * WIFI_SCAN_HOTSPOT_LIMIT);
    if (info == NULL) {
        return -1;
    }
    // 轮询查找WiFi列表
    do {
        Scan();
        WaitSacnResult();
        error = GetScanInfoList(info, &size);
    } while (g_staScanSuccess != 1);
    // 打印WiFi列表
    printf("********************\r\n");
    for (uint8_t i = 0; i < g_ssid_count; i++) {
        printf("no:%03d, ssid:%-30s, rssi:%5d\r\n", i + 1, info[i].ssid, info[i].rssi);
    }
    printf("********************\r\n");
    // 连接指定的WiFi热点
    for (uint8_t i = 0; i < g_ssid_count; i++) {
        if (WifiConnectAp(SELECT_WIFI_SSID, SELECT_WIFI_PASSWORD, info, i) == WIFI_SUCCESS) {
            printf("WiFi connect succeed!\r\n");
            break;
        }

        if (i == g_ssid_count - 1) {
            printf("ERROR: No wifi as expected\r\n");
            while (1)
                osDelay(DHCP_DELAY);
        }
    }

    // 启动DHCP
    if (g_lwip_netif) {
        dhcp_start(g_lwip_netif);
        printf("begain to dhcp\r\n");
    }
    // 等待DHCP
    for (;;) {
        if (dhcp_is_bound(g_lwip_netif) == ERR_OK) {
            printf("<-- DHCP state:OK -->\r\n");
            // 打印获取到的IP信息
            netifapi_netif_common(g_lwip_netif, dhcp_clients_info_show, NULL);
            break;
        }
        osDelay(DHCP_DELAY);
    }

}

int WiFiInit(void)
{
    g_wifiEventHandler.OnWifiScanStateChanged = OnWifiScanStateChangedHandler;
    g_wifiEventHandler.OnWifiConnectionChanged = OnWifiConnectionChangedHandler;
    g_wifiEventHandler.OnHotspotStaJoin = OnHotspotStaJoinHandler;
    g_wifiEventHandler.OnHotspotStaLeave = OnHotspotStaLeaveHandler;
    g_wifiEventHandler.OnHotspotStateChanged = OnHotspotStateChangedHandler;
    error = RegisterWifiEvent(&g_wifiEventHandler);
    if (error != WIFI_SUCCESS) {
        printf("register wifi event fail!\r\n");
        return -1;
    }
    // 使能WIFI
    if (EnableWifi() != WIFI_SUCCESS) {
        printf("EnableWifi failed, error = %d\r\n", error);
        return -1;
    }
    // 判断WIFI是否激活
    if (IsWifiActive() == 0) {
        printf("Wifi station is not actived.\r\n");
        return -1;
    }
    return 0;
}

static void OnWifiScanStateChangedHandler(int state, int size)
{
    (void)state;
    if (size > 0) {
        g_ssid_count = size;
        g_staScanSuccess = 1;
    }
    return;
}

static void OnWifiConnectionChangedHandler(int state, WifiLinkedInfo *info)
{
    (void)info;

    if (state > 0) {
        g_connectSuccess = 1;
        printf("callback function for wifi connect\r\n");
    } else {
        printf("connect error,please check password\r\n");
    }
    return;
}

static void OnHotspotStaJoinHandler(StationInfo *info)
{
    (void)info;
    printf("STA join AP\n");
    return;
}

static void OnHotspotStaLeaveHandler(StationInfo *info)
{
    (void)info;
    printf("HotspotStaLeave:info is null.\n");
    return;
}

static void OnHotspotStateChangedHandler(int state)
{
    printf("HotspotStateChanged:state is %d.\n", state);
    return;
}

static void WaitSacnResult(void)
{
    int scanTimeout = DEF_TIMEOUT;
    while (scanTimeout > 0) {
        sleep(ONE_SECOND);
        scanTimeout--;
        if (g_staScanSuccess == 1) {
            printf("WaitSacnResult:wait success[%d]s\n", (DEF_TIMEOUT - scanTimeout));
            break;
        }
    }
    if (scanTimeout <= 0) {
        printf("WaitSacnResult:timeout!\n");
    }
}

static int WaitConnectResult(void)
{
    int ConnectTimeout = DEF_TIMEOUT;
    while (ConnectTimeout > 0) {
        sleep(1);
        ConnectTimeout--;
        if (g_connectSuccess == 1) {
            printf("WaitConnectResult:wait success[%d]s\n", (DEF_TIMEOUT - ConnectTimeout));
            break;
        }
    }
    if (ConnectTimeout <= 0) {
        printf("WaitConnectResult:timeout!\n");
        return 0;
    }

    return 1;
}

/*static BOOL WifiAPTask(void)
{
    // 延时2S便于查看日志
    osDelay(TASK_DELAY_2S);

    // 注册wifi事件的回调函数
    g_wifiEventHandler.OnHotspotStaJoin = OnHotspotStaJoinHandler;
    g_wifiEventHandler.OnHotspotStaLeave = OnHotspotStaLeaveHandler;
    g_wifiEventHandler.OnHotspotStateChanged = OnHotspotStateChangedHandler;
    error = RegisterWifiEvent(&g_wifiEventHandler);
    if (error != WIFI_SUCCESS) {
        printf("RegisterWifiEvent failed, error = %d.\r\n", error);
        return -1;
    }
    printf("RegisterWifiEvent succeed!\r\n");
    // 检查热点模式是否使能
    if (IsHotspotActive() == WIFI_HOTSPOT_ACTIVE) {
        printf("Wifi station is  actived.\r\n");
        return -1;
    }
    // 设置指定的热点配置
    HotspotConfig config = { 0 };

    strcpy_s(config.ssid, strlen(AP_SSID) + 1, AP_SSID);
    strcpy_s(config.preSharedKey, strlen(AP_PSK) + 1, AP_PSK);
    config.securityType = WIFI_SEC_TYPE_PSK;
    config.band = HOTSPOT_BAND_TYPE_2G;
    config.channelNum = CHANNEL_NUM;

    error = SetHotspotConfig(&config);
    if (error != WIFI_SUCCESS) {
        printf("SetHotspotConfig failed, error = %d.\r\n", error);
        return -1;
    }
    printf("SetHotspotConfig succeed!\r\n");

    // 启动wifi热点模式
    error = EnableHotspot();
    if (error != WIFI_SUCCESS) {
        printf("EnableHotspot failed, error = %d.\r\n", error);
        return -1;
    }
    printf("EnableHotspot succeed!\r\n");

    StartUdpServer();
}*/

static void WifiClientSTA(void)
{
    osThreadAttr_t attr;

    attr.name = "WifiStaTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = TASK_STACK_SIZE;
    attr.priority = 24;

    if (osThreadNew((osThreadFunc_t)WifiStaTask, NULL, &attr) == NULL) {
        printf("Failed to create WifiStaTask!\n");
    }
}

APP_FEATURE_INIT(WifiClientSTA);
