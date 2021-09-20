#include "pmdr_wifi.h"
#include "pmdr_config.h"
#include "wifi_device.h"
#include "wifi_device_config.h"
#include "securec.h"
#include "pmdr_def.h"
#include "pmdr_error.h"
#include "pmdr_log.h"
#include "pmdr_os.h"

#define CONNECT_MAX_TIME    (60 * 1000)
#define SCAN_MAX_TIME       (30 * 1000)

static char g_wifiSsid[WIFI_MAX_SSID_LEN] = {0};
static char g_wifiPwd[WIFI_MAX_KEY_LEN] = {0};

static bool g_isWifiNeedInit = true;

static bool g_isConnectFinsh = false;
static unsigned long g_connectTime = 0;

static bool g_isScanFinsh = false;
static unsigned long g_scanTime = 0;

typedef enum {
    PMDR_WIFI_STATE_INVALID = -1,
    PMDR_WIFI_STATE_INIT,
    PMDR_WIFI_SCAN,
    PDMD_WIFI_WAITING_SCAN,
    PMDR_WIFI_CONNECT,
    PMDR_WIFI_WAITING_CONNECT,
    PMDR_WIFI_FINISH,
    PMDR_WIFI_FAILED,
} PmdrWifiState;

static PmdrWifiState g_wifiState = PMDR_WIFI_STATE_INVALID;

static PmdrWifiState GetWifiState(void)
{
    return g_wifiState;
}

static void SetWifiState(PmdrWifiState state)
{
    g_wifiState = state;
    PMDR_LOG_INFO("set wifi state[%d]", state);
    return;
}

static int GetWifiInfo(void)
{
    /* 当前不需要配网 */
    (void)memset_s(g_wifiSsid, WIFI_MAX_SSID_LEN, 0, WIFI_MAX_SSID_LEN);
    (void)memset_s(g_wifiPwd, WIFI_MAX_KEY_LEN, 0, WIFI_MAX_KEY_LEN);
    if (strcpy_s(g_wifiSsid, sizeof(g_wifiSsid), PMDR_WIFI_SSID) != EOK) {
        PMDR_LOG_ERROR("strcpy_s error");
        return PMDR_ERROR;
    }
    if (strcpy_s(g_wifiPwd, sizeof(g_wifiPwd), PMDR_WIFI_PWD) != EOK) {
        PMDR_LOG_ERROR("strcpy_s error");
        return PMDR_ERROR;
    }
    return PMDR_OK;
}

static void OnWifiConnectionChanged(int state, WifiLinkedInfo* info)
{
    (void)info;
    PMDR_LOG_PRINT("wifi state change[%d]", state);
    if (state == WIFI_STATE_AVALIABLE) {
        g_isConnectFinsh = true;
    }
}

static void OnWifiScanStateChanged(int state, int size)
{
    PMDR_INFO("wifi scan finish state[%d] size[%d]", state, size);
    if (state == WIFI_STATE_AVALIABLE) {
        g_isScanFinsh = true;
    }
}

static int WifiConectInit(void)
{
    /* 激活wifi */
    if (IsWifiActive() == WIFI_STA_NOT_ACTIVE && EnableWifi() != WIFI_SUCCESS) {
        PMDR_LOG_ERROR("wifi enable failed");
        return PMDR_ERROR;
    }

    WifiEvent event = {
        OnWifiConnectionChanged,
        OnWifiScanStateChanged,
        NULL,
        NULL,
        NULL
    };
    /* 注册wifi回调 */
    if (RegisterWifiEvent(&event) != WIFI_SUCCESS) {
        PMDR_LOG_ERROR("register wifi event failed");
        return PMDR_ERROR;
    }

    return PMDR_OK;
}

static int WifiScan(void)
{
    if (Scan() != WIFI_SUCCESS) {
        PMDR_LOG_ERROR("wifi scan failed");
        return PMDR_ERROR;
    }
    return PMDR_OK;
}

static bool IsScanOverTime(void)
{
    if (g_scanTime == 0 && PmdrGetTimeCount(&g_scanTime) != PMDR_OK) {
        PMDR_LOG_ERROR("scan time error");
        return true;
    }

    unsigned long curTime;
    if (PmdrGetTimeCount(&curTime) != PMDR_OK) {
        PMDR_LOG_ERROR("get time error");
        return true;
    }

    if (curTime > g_scanTime + SCAN_MAX_TIME) {
        PMDR_LOG_WARNING("scan over time");
        return true;
    }
    return false;
}

static int WaitScanResult(void)
{
    if (!g_isScanFinsh) {
        return IsScanOverTime() ? PMDR_ERROR : PMDR_PASS;
    }
    return PMDR_OK;
}

static int GetNetId(const WifiDeviceConfig *config)
{
    WifiDeviceConfig *allConfig = (WifiDeviceConfig *)malloc(sizeof(WifiDeviceConfig) * WIFI_MAX_CONFIG_SIZE);
    if (allConfig == NULL) {
        PMDR_LOG_ERROR("malloc error");
        return -1;
    }
    unsigned int size = 0;
    if (GetDeviceConfigs(allConfig, &size) != WIFI_SUCCESS) {
        PMDR_LOG_ERROR("get config error");
        free(allConfig);
        allConfig = NULL;
        return -1;
    }
    unsigned int index;
    for (index = 0; index < size; ++index) {
        if ((strlen(allConfig[index].ssid) == strlen(config->ssid)) &&
            (strlen(allConfig[index].preSharedKey) == strlen(config->preSharedKey)) &&
            (strncmp(allConfig[index].ssid, config->ssid, strlen(config->ssid)) == 0) &&
            (strncmp(allConfig[index].preSharedKey, config->preSharedKey, strlen(config->preSharedKey)) == 0) &&
            (memcmp(allConfig[index].bssid, config->bssid, WIFI_MAC_LEN) == 0)) {
            break;
        }
    }
    if (index != size) {
        free(allConfig);
        allConfig = NULL;
        PMDR_LOG_INFO("find config, netid[%d]", allConfig[index].netId);
        return allConfig[index].netId;
    }
    /* 如果找不到已存配置，且配置已满，则删除第一个配置文件 */
    if (size == WIFI_MAX_CONFIG_SIZE) {
        PMDR_LOG_INFO("config full, remove first one");
        if (RemoveDevice(allConfig[0].netId) != WIFI_SUCCESS) {
            free(allConfig);
            allConfig = NULL;
            PMDR_LOG_ERROR("remove config error");
            return -1;
        }
    }
    free(allConfig);
    allConfig = NULL;

    int netId = 0;
    if (AddDeviceConfig(config, &netId) != WIFI_SUCCESS) {
        PMDR_LOG_ERROR("add wifi config error");
        return -1;
    }
    
    return netId;
}

static int WifiConnect(void)
{
    WifiScanInfo *scanList = (WifiScanInfo *)malloc(sizeof(WifiScanInfo) * WIFI_SCAN_HOTSPOT_LIMIT);
    if (scanList == NULL) {
        PMDR_LOG_ERROR("malloc error");
        return PMDR_ERROR;
    }
    unsigned int scanSize = WIFI_SCAN_HOTSPOT_LIMIT;

    if (GetScanInfoList(scanList, &scanSize) != WIFI_SUCCESS) {
        PMDR_LOG_ERROR("get scan list error");
        free(scanList);
        scanList = NULL;
        return PMDR_ERROR;
    }

    unsigned int index;
    for (index = 0; index < scanSize; ++index) {
        if ((strlen(scanList[index].ssid) == strlen(g_wifiSsid)) &&
            (strncmp(scanList[index].ssid, g_wifiSsid, strlen(g_wifiSsid)) == 0)) {
            break;
        }
    }
    if (index == scanSize) {
        PMDR_LOG_WARNING("not find target ssid");
        free(scanList);
        scanList = NULL;
        return PMDR_ERROR;
    } else {
        PMDR_LOG_INFO("find target ssid");
    }

    WifiDeviceConfig config;
    (void)memset_s(&config, sizeof(WifiDeviceConfig), 0, sizeof(WifiDeviceConfig));
    config.freq = scanList[index].frequency;
    config.securityType = scanList[index].securityType;
    config.wapiPskType = WIFI_PSK_TYPE_ASCII;
    memcpy_s(config.bssid, WIFI_MAC_LEN, scanList[index].bssid, WIFI_MAC_LEN);
    memcpy_s(config.ssid, WIFI_MAX_SSID_LEN, g_wifiSsid, WIFI_MAX_SSID_LEN);
    memcpy_s(config.preSharedKey, WIFI_MAX_KEY_LEN, g_wifiPwd, WIFI_MAX_KEY_LEN);

    int netId = GetNetId(&config);
    if (netId < 0) {
        free(scanList);
        scanList = NULL;
        return PMDR_ERROR;
    }
    
    PMDR_LOG_PRINT("connect to ssid:[%s]", config.ssid);
    if (ConnectTo(netId) != WIFI_SUCCESS) {
        free(scanList);
        scanList = NULL;
        PMDR_LOG_ERROR("connect to wifi error");
        return PMDR_ERROR;
    }

    return PMDR_OK;
}

static bool IsConnectOverTime(void)
{
    if (g_connectTime == 0 && PmdrGetTimeCount(&g_connectTime) != PMDR_OK) {
        PMDR_LOG_ERROR("connect time error");
        return true;
    }

    unsigned long curTime;
    if (PmdrGetTimeCount(&curTime) != PMDR_OK) {
        PMDR_LOG_ERROR("get time error");
        return true;
    }

    if (curTime > g_connectTime + CONNECT_MAX_TIME) {
        PMDR_LOG_WARNING("connect over time");
        return true;
    }
    return false;
}

static int WaitWifiConnect(void)
{
    if (!g_isConnectFinsh) {
        return IsConnectOverTime() ? PMDR_ERROR : PMDR_PASS;
    }
    return PMDR_OK;
}

static void ResetWifiConnect(void) {
    g_isWifiNeedInit = true;
    g_isConnectFinsh = false;
    g_connectTime = 0;
    g_isScanFinsh = false;
    g_scanTime = 0;
    g_wifiState = PMDR_WIFI_STATE_INIT;
}

/* 返回PMDR_OK或PMDR_ERROR退出状态机 */
int PmdrWifiConnctProcess(void)
{
    int ret;
    static bool isGetWifiInfo = false;
    /* 配网 */
    if (!isGetWifiInfo) {
        ret = GetWifiInfo();
        if (ret == PMDR_OK) {
            isGetWifiInfo = true;
        } else if (ret == PMDR_ERROR) {
            PMDR_LOG_ERROR("get wifi info failed");
            return PMDR_ERROR;
        }
    }

    if (g_isWifiNeedInit) {
        SetWifiState(PMDR_WIFI_STATE_INIT);
        g_isWifiNeedInit = false;
    }
    int err = PMDR_PASS;
    /* 联网 */
    PmdrWifiState state = GetWifiState();
    switch (state) {
        case PMDR_WIFI_STATE_INIT:
            ret = WifiConectInit();
            SetWifiState((ret == PMDR_OK) ? PMDR_WIFI_SCAN : PMDR_WIFI_FAILED);
            break;
        case PMDR_WIFI_SCAN:
            ret = WifiScan();
            SetWifiState((ret == PMDR_OK) ? PDMD_WIFI_WAITING_SCAN : PMDR_WIFI_FAILED);
            break;
        case PDMD_WIFI_WAITING_SCAN:
            ret = WaitScanResult();
            if (ret != PMDR_PASS) {
                SetWifiState((ret == PMDR_OK) ? PMDR_WIFI_CONNECT : PMDR_WIFI_FAILED);
            }
            break;
        case PMDR_WIFI_CONNECT:
            ret = WifiConnect();
            SetWifiState((ret == PMDR_OK) ? PMDR_WIFI_WAITING_CONNECT : PMDR_WIFI_FAILED);
            break;
        case PMDR_WIFI_WAITING_CONNECT:
            ret = WaitWifiConnect();
            if (ret != PMDR_PASS) {
                SetWifiState((ret == PMDR_OK) ? PMDR_WIFI_FINISH : PMDR_WIFI_FAILED);
            }
            break;
        case PMDR_WIFI_FINISH:
            ResetWifiConnect();
            err = PMDR_OK;
            break;
        case PMDR_WIFI_FAILED:
            ResetWifiConnect();
            err = PMDR_ERROR;
            break;
        default:
            PMDR_LOG_ERROR("state invalid");
            break;
    }
    
    return err;
}