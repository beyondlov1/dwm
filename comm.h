// unused
#ifndef __COMM_H__
#define __COMM_H__
#include <stdio.h>
#include <stdlib.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define IPCKEY 0x366378

int writecomm(int);
int readcomm(int *);
int clearcomm();
#endif