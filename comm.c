// unused
#include "comm.h"

int 
writecomm(int a){
    int shm_id;

    int *dataptr;

    //  首先检查共享内存是否存在，存在则先删除
    shm_id = shmget(IPCKEY, 4, 0640);
    if (shm_id != -1)
    {
        dataptr = (int *)shmat(shm_id, NULL, 0);
        if (dataptr != (void *)-1)
        {
            shmdt(dataptr);
            shmctl(shm_id, IPC_RMID, 0);
        }
    }

    //  创建共享内存
    shm_id = shmget(IPCKEY, 4, 0640 | IPC_CREAT | IPC_EXCL);
    if (shm_id == -1)
    {
        printf("shmget error\n");
        return -1;
    }

    //  将这块共享内存区附加到自己的内存段
    dataptr = (int *)shmat(shm_id, NULL, 0);

    *dataptr = a;

    // printf("%p", dataptr);

    return 0;
}

int 
readcomm(int *dataptr)
{
    int shm_id;

    //  创建共享内存
    shm_id = shmget(IPCKEY, 4, 0640 | IPC_CREAT);
    if (shm_id == -1)
    {
        printf("shmget error\n");
        return -1;
    }

    //  将这块共享内存区附加到自己的内存段
    int *result = (int *)shmat(shm_id, NULL, 0);

    // printf("%p", result);
    *dataptr = *result;

    return 0;
}

int
clearcomm(){
    int shm_id;

    //  创建共享内存
    shm_id = shmget(IPCKEY, 4, 0640 | IPC_CREAT);
    if (shm_id == -1)
    {
        printf("shmget error\n");
        return -1;
    }

    int *result = (int *)shmat(shm_id, NULL, 0);
    //  将这块共享内存区从自己的内存段删除出去
    if (shmdt(result) == -1)
        perror(" detach error ");

    //  删除共享内存
    if (shmctl(shm_id, IPC_RMID, NULL) == -1)
    {
        perror(" delete error ");
    }
    return 0;
}