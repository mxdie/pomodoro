#ifndef PMDR_ERROR_H
#define PMDR_ERROR_H

typedef enum {
    PMDR_PASS                   = 1,
    PMDR_OK                     = 0,
    PMDR_ERROR                  = -1,
    /* -100 ~ -200 操作系统 */
    PMDR_SYS_TIME_ERROR         = -100, /* 系统时间错误 */
    PMDR_SECUREC_ERROR          = -101, /* 安全函数报错 */
} PmdrErrorCode;

#endif /* PMDR_ERROR_H */