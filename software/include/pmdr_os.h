#ifndef PMDR_OS_H
#define PMDR_OS_H

int PmdrOsInit(void);

void PmdrSleep(unsigned int ms);

int PmdrGetTimeCount(unsigned long *time);

#endif