#ifndef PMDR_CONFIG_H
#define PMDR_CONFIG_H

/* --------------任务属性配置-------------- */
/* 任务栈大小 */
#define PMDR_TASK_SIZE      (8192)
/* 主状态机间隔，单位ms */
#define PMDR_TASK_INTERVAL  50
/* --------------------------------------- */


/* -------------日志打印等级配置------------ */
#define PMDR_LOG_LEVEL_CLOSE    0
#define PMDR_LOG_LEVEL_PRINT    1
#define PMDR_LOG_LEVEL_ERROR    2
#define PMDR_LOG_LEVEL_WARNING  3
#define PMDR_LOG_LEVEL_INFO     4
#define PMDR_LOG_LEVEL_DEBUG    5

#define PMDR_LOG_LEVEL  PMDR_LOG_LEVLE_DEBUG
/* --------------------------------------- */

/* ---------------wifi账号密码------------- */
#define PMDR_WIFI_SSID      "test"
#define PMDR_WIFI_PWD       "test"
/* --------------------------------------- */

#endif /* PMDR_CONFIG_H */