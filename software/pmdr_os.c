#include "pmdr_os.h"
#include "cmsis_os2.h"
#include "pmdr_error.h"
#include "pmdr_log.h"

#define MS_PER_SECOND   1000
static unsigned int g_msPerTick = 1;

static int GetMsPerTick(void)
{
    unsigned int tickFreq = osKernelGetTickFreq();
    if (tickFreq == 0) {
        PMDR_LOG_ERROR("tick freq is 0!!!");
        return PMDR_ERROR;
    }
    /* 系统频率要能整除1000，这里3861为100 */
    g_msPerTick = MS_PER_SECOND / tickFreq;

    return PMDR_OK;
}

/* 为初始化前sleep以tick为单位，初始化后以ms为单位 */
void PmdrSleep(unsigned int ms)
{
    unsigned int tick;
    tick =  ms / g_msPerTick;
    osDelay(tick);
}

int PmdrGetTimeCount(unsigned long *time)
{
    unsigned int tickCount = osKernelGetTickCount();
    if (tickCount == 0) {
        PMDR_LOG_ERROR("get tick count error");
        return PMDR_ERROR;
    }
    *time = (unsigned long)g_msPerTick * (unsigned long)tickCount;
    return PMDR_OK;
}

int PmdrOsInit(void)
{
    if (GetMsPerTick() != PMDR_OK) {
        return PMDR_ERROR;
    }

    return PMDR_OK;
}