#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/timeb.h>
#include <time.h>

#define LENGTH(X) (sizeof X / sizeof X[0])

int getppidof(int pid)
{
    char dir[1024]={0};
    char path[1028] = {0};
    char buf[1024] = {0};
    int rpid = 0;
    int fpid=0;
    char fpth[1024]={0};
    ssize_t ret =0;
    sprintf(dir,"/proc/%d/",pid);
    sprintf(path,"%sstat",dir);
    memset(buf,0,strlen(buf));
    FILE * fp = fopen(path,"r");
    ret += fread(buf + ret,1,300-ret,fp);
    fclose(fp);
    sscanf(buf,"%*d %*c%s %*c %d %*s",fpth,&fpid);
    fpth[strlen(fpth)-1]='\0';
    return fpid;
}

typedef struct ScratchItem ScratchItem;
struct ScratchItem
{
	int tags;
	int pretags;
	ScratchItem *next;
	ScratchItem *prev;
};


ScratchItem *
alloc_si()
{
	ScratchItem * si = (ScratchItem *)malloc(sizeof(ScratchItem));
	memset(si,0,sizeof(ScratchItem));
	return si;
}

int cmp(ScratchItem *a, ScratchItem *b)
{
    return a->tags - b->tags;
}

void sort(void *clist[], size_t n, int (*cmp)(const void *, const void *))
{
    int i;
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = i + 1; j < n; j++)
        {
            if (cmp(clist[j], clist[i]) < 0)
            {
                void *tmp = clist[i];
                clist[i] = clist[j];
                clist[j] = tmp;
            }
        }
    }
}

int main(int argc, char const *argv[])
{

    int aaa[5] = {1,2,3,4,5,6};
    printf("%p", aaa[2]);
    printf("%p", aaa[3]);
    printf("%p", aaa[4]);
    printf("%p", aaa[5]);

	int brr[5] = { 6,7,8,9,10 };
    int arr[5] = { 1,2,3,4,5 };
	brr[7] = 20;
	printf("%d\n", arr[0]);
    printf("%d\n", arr[1]);
    printf("%d\n", arr[2]);
    printf("%d\n", arr[3]);

    time_t tnow;
    tnow=time(0);      // 获取当前时间
    printf("tnow=%lu\n",tnow);   // 输出整数表示的时间

    struct tm *sttm;  
    sttm=localtime(&tnow);  // 把整数的时间转换为struct tm结构体的时间

    // yyyy-mm-dd hh24:mi:ss格式输出，此格式用得最多
    printf("%04u-%02u-%02u %02u:%02u:%02u\n",sttm->tm_year+1900,sttm->tm_mon+1,\
            sttm->tm_mday,sttm->tm_hour,sttm->tm_min,sttm->tm_sec);

    printf("%04u年%02u月%02u日%02u时%02u分%02u秒\n",sttm->tm_year+1900,\
            sttm->tm_mon+1,sttm->tm_mday,sttm->tm_hour,sttm->tm_min,sttm->tm_sec);

    // 只输出年月日
    printf("%04u-%02u-%02u\n",sttm->tm_year+1900,sttm->tm_mon+1,sttm->tm_mday);
    
    ScratchItem a1 = {.tags = 1};
    ScratchItem a2 = {.tags = 2};
    ScratchItem a3 = {.tags = 3};
    ScratchItem a4 = {.tags = 4};

    // printf("a");

    ScratchItem *alist[4];
    alist[0] = &a3;
    alist[1] = &a2;
    alist[2] = &a4;
    alist[3] = &a1;

    sort(alist, 4, cmp);

    for (size_t i = 0; i < 4; i++)
    {
        printf("a: %d", alist[i]->tags);
    }
    

    struct timeval us;
    gettimeofday(&us,NULL);
    printf("gettimeofday: tv_sec=%ld, tv_usec=%ld\n", us.tv_sec, us.tv_usec);

    struct timespec time;
    printf("%lld.%.9ld seconds have elapsed!", (long long) time.tv_sec, time.tv_nsec);
    printf("\nOR \n%ld seconds and %ld nanoseconds have elapsed!", time.tv_sec, time.tv_nsec);
    
    struct timeb t;
    ftime(&t);
    printf("%ld %d", t.time, t.millitm);
    int a = 1;
    a |= 0;
    // int b = getppidof(420724);
    // printf("%d", b);
    alloc_si();
    printf("%f", log(1.0/2));
    return 0;
}
