#include "ohos_init.h"
#include "cmsis_os2.h"
#include "pmdr_config.h"
#include "pmdr_log.h"
#include "pmdr_def.h"
#include "pmdr_os.h"
#include "pmdr_wifi.h"
#include "pmdr_error.h"
#include "securec.h"
#include "lwip/sntp.h" 
#include "lwip/err.h"

typedef enum {
    PMDR_MAIN_STATE_INVALID = -1,
    PMDR_MAIN_STATE_INTI,
    PMDR_MAIN_STATE_CONNECT_WIFI,
    PMDR_MAIN_STATE_GET_TIME,
    PMDR_MAIN_STATE_FINSH
} PmdrMainState;

static PmdrMainState g_mainState = PMDR_MAIN_STATE_INTI;

static PmdrMainState GetMainState(void)
{
    return g_mainState;
}

static void SetMainState(PmdrMainState state)
{
    PMDR_LOG_INFO("set main state [%d]", state);
    g_mainState = state;
    return;
}


static int PmdrGetTime(void)
{
    int ret;
    int serverNum = 1;
    char *sntpServer = SNTP_SERVER_IP;
    struct timeval timeLocal;
    memset_s(&timeLocal, 0, sizeof(timeLocal), 0);
    extern int lwip_sntp_start(int server_num, char **sntp_server, struct timeval *time);
    ret = lwip_sntp_start(serverNum, &sntpServer, &timeLocal);
    PMDR_LOG_PRINT("Recevied time from server = [%li]sec [%li]u sec\n", timeLocal.tv_sec, timeLocal.tv_usec);
    if (ret != ERR_OK) {
        PMDR_LOG_ERROR("lwip sntp error");
        return PMDR_ERROR;
    }
    PMDR_LOG_PRINT("Recevied time from server = [%li]sec [%li]u sec\n", timeLocal.tv_sec, timeLocal.tv_usec);
    return PMDR_OK;
}

int PmdrMainProcess(void)
{
    int ret = PMDR_PASS;
    int err;
    PmdrMainState state = GetMainState();
    switch (state) {
        case PMDR_MAIN_STATE_INTI:
            SetMainState(PMDR_MAIN_STATE_CONNECT_WIFI);
            break;
        case PMDR_MAIN_STATE_CONNECT_WIFI:
            err = PmdrWifiConnctProcess();
            if (err != PMDR_PASS) {
                SetMainState((err == PMDR_OK) ? PMDR_MAIN_STATE_GET_TIME :
                    PMDR_MAIN_STATE_INTI);
            }
            break;
        case PMDR_MAIN_STATE_GET_TIME:
            err = PmdrGetTime();
            break;
        case PMDR_MAIN_STATE_FINSH:
            ret = PMDR_ERROR;
            break;
        default:
            PMDR_LOG_ERROR("invalid state[%d]", state);
            break;
    }
    return ret;
}

int MainTaskInit(void)
{
    if (PmdrOsInit() != PMDR_OK) {
        PMDR_LOG_ERROR("os init error");
        return PMDR_ERROR;
    }
    SetMainState(PMDR_MAIN_STATE_INTI);
    return PMDR_OK;
}

void PmdrAppMain(void *arg)
{
    (void)arg;
    /* sleep 100???tick????????????????????????????????? */
    osDelay(100);
    PMDR_LOG_PRINT("pomodoro start");
    if (MainTaskInit() != PMDR_OK) {
        PMDR_LOG_ERROR("main task init error");
        return;
    }   

    while (PmdrMainProcess() == PMDR_PASS) {
        PmdrSleep(PMDR_TASK_INTERVAL);
    }
    PMDR_LOG_PRINT("pomodoro exit");
}

void PmdrAppMainTask(void)
{
    osThreadAttr_t attr;
    (void)memset_s(&attr, sizeof(osThreadAttr_t), 0, sizeof(osThreadAttr_t));
    attr.name = "PmdrTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = PMDR_TASK_SIZE;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)PmdrAppMain, NULL, &attr) == NULL) {
        PMDR_LOG_ERROR("failed to create pmdr task");
    }
}

SYS_RUN(PmdrAppMainTask);