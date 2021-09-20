/* 日志实现 */
#ifndef PMDR_LOG_H
#define PMDR_LOG_H

#include "pmdr_config.h"

#ifndef PMDR_LOG_LEVEL
#define PMDR_LOG_LEVEL  PMDR_LOG_LEVEL_DEBUG
#endif

void PdmrLogOutput(const char *msg, ...);

#if (PMDR_LOG_LEVEL >= PMDR_LOG_LEVEL_PRINT)
#define PMDR_LOG_PRINT(msg,...)                             \
do{                                                     \
    PdmrLogOutput("PRINT|%s|%d:",__FUNCTION__, __LINE__);      \
    PdmrLogOutput(msg, ##__VA_ARGS__);                       \
    PdmrLogOutput("\r\n");                                     \
}while(0)
#else
#define PMDR_LOG_PRINT(msg,...) do {} while(0)
#endif

#if (PMDR_LOG_LEVEL >= PMDR_LOG_LEVEL_ERROR)
#define PMDR_LOG_ERROR(msg,...)                             \
do{                                                     \
    PdmrLogOutput("ERROR|%s|%d:",__FUNCTION__, __LINE__);      \
    PdmrLogOutput(msg, ##__VA_ARGS__);                       \
    PdmrLogOutput("\r\n");                                     \
}while(0)
#else
#define PMDR_LOG_ERROR(msg,...) do {} while(0)
#endif

#if (PMDR_LOG_LEVEL >= PMDR_LOG_LEVEL_WARNING)
#define PMDR_LOG_WARNING(msg,...)                            \
do{                                                     \
    PdmrLogOutput("WARING|%s|%d:",__FUNCTION__, __LINE__);     \
    PdmrLogOutput(msg, ##__VA_ARGS__);                       \
    PdmrLogOutput("\r\n");                                     \
}while(0)
#else
#define PMDR_LOG_WARNING(msg,...) do {} while(0)
#endif

#if (PMDR_LOG_LEVEL >= PMDR_LOG_LEVEL_INFO)
#define PMDR_LOG_INFO(msg,...)                              \
do{                                                     \
    PdmrLogOutput("INFO|%s|%d:",__FUNCTION__, __LINE__);       \
    PdmrLogOutput(msg, ##__VA_ARGS__);                       \
    PdmrLogOutput("\r\n");                                     \
}while(0)
#else
#define PMDR_LOG_INFO(msg,...) do {} while(0)
#endif

#if (PMDR_LOG_LEVEL >= PMDR_LOG_LEVEL_DEBUG)
#define PMDR_LOG_DEBUG(msg,...)                             \
do{                                                     \
    PdmrLogOutput("DEBUG|%s|%d:",__FUNCTION__, __LINE__);      \
    PdmrLogOutput(msg, ##__VA_ARGS__);                       \
    PdmrLogOutput("\r\n");                                     \
}while(0)
#else
#define PMDR_LOG_DEBUG(msg,...) do {} while(0)
#endif

#endif /* PMDR_LOG_H */