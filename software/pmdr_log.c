#include "pmdr_log.h"
#include <stdio.h> 
#include <stdarg.h>

void PdmrLogOutput(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    vprintf(msg, args);
    va_end(args);
}
