#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/timeb.h>

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

int main(int argc, char const *argv[])
{
    struct timeval us;
    gettimeofday(&us,NULL);
    printf("gettimeofday: tv_sec=%ld, tv_usec=%ld\n", us.tv_sec, us.tv_usec);

    struct timespec time;
    printf("%lld.%.9ld seconds have elapsed!", (long long) time.tv_sec, time.tv_nsec);
    printf("\nOR \n%ld seconds and %ld nanoseconds have elapsed!", time.tv_sec, time.tv_nsec);
    return 0;
    
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
