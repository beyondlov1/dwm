#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdarg.h>

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

int readstruct2()
{
        
	FILE *fp = fopen("tmp.csv", "r");
	if (fp == NULL) {
		fprintf(stderr, "fopen() failed.\n");
		exit(EXIT_FAILURE);
	}
	while(!feof(fp))
	{
		char a1[30];
		char a2[30];
		char a3[30];
		char a4[30];
		char a5[30];
	    fscanf(fp, "%s	%s	%s	%s	%s", a1,a2,a3,a4,a5);
	    printf("%s-%s-%s-%s-%s", a1, a2,a3,a4,a5);
	}
	fclose(fp);
	return 0;
}

int readstruct()
{
    FILE *fp = fopen("tmp.csv", "r");
    if (fp == NULL) {
        fprintf(stderr, "fopen() failed.\n");
        exit(EXIT_FAILURE);
    }

    char row[80];
    char *token;

    while (fgets(row, 80, fp) != NULL) {
        printf("Row: %s", row);
        token = strtok(row, "	"); 
        while (token != NULL) {
            printf("Token: %s\n", token);
            token = strtok(NULL, "	");
        }
    }

    fclose(fp);
    return 0;
}

void 
right(int **arr, int row ,int col, int x, int y, int result[2])
{
	if (y == col - 1) {
		return;
	}
	int j;
	for(j = y + 1;j <col;j++)
	{
		int step;
		int i;
		for(step = 0; step < row; step ++)
		{
			i = x + step;
			printf("i,j:%d %d",i,j);
			int m = *((int *)arr + col*i + j);
			printf("m:%d",m);
			if (i >= 0 && i < row) {
				if(m == 1)
				{
					printf("%d %d\n", i, j);
					fflush(stdout);
					result[0] = i;
					result[1] = j;
					return;
				}
			}
			i = x - step;
			m = *((int*)arr + row*i + j);
			if (i >= 0 && i < row) {
				if(m == 1)
				{
					printf("%d %d\n", i, j);
					fflush(stdout);
					result[0] = i;
					result[1] = j;
					return;
				}
			}
			printf("%d %d\n", i, j);
			fflush(stdout);
		}
	}
}

void test()
{
	fflush(stdout);
	int selcurtagindex = 0;
	const int row = 3;
	const int col = 3;
	int arr[3][3] ={
		{1,0,0},
		{1,1,0},
		{0,0,0}};
	int i;
	int j;
	int x = selcurtagindex/row;
	int y = selcurtagindex%row;
	int result[2];
	result[0] = x;
	result[1] = y;

	right(arr, row, col, x, y, result);
	selcurtagindex = result[0] * row + result[1];
	
	printf("%d %d", result[0], result[1]);
	fflush(stdout);
}


int 
mmax(int num, ...)
{
	va_list ap;
	va_start(ap,num);
	int result = 0;
	int temp = 0;
	int i;
	for (i = 0; i < num; i++) {
		temp = va_arg(ap, int); 
		result = result > temp ? result:temp;
	}
	va_end(ap);
	return result;
}


struct Client{
	int i;
	struct Client *next;
};
typedef struct Client Client ;


int
test2(){
	int i = 16;
	
	char candidates[][10] = {
		{'h','j','k','l','p','u','n','m', 'i', 'c'},
		{'s','a','d','f'},
		{'g','r','e','w'},
	};
	int candidatesns[] = {10,4,4};
	int N = 3;
	int lvlstart[N];
	lvlstart[0] = 0;
	for(int b=1;b<N+1;b++)
	{
		int m = 1;
		for(int d=0;d<b;d++) m*= candidatesns[d];
		lvlstart[b] = lvlstart[b-1] + m;
	}
	int a;
	for (a = 0; a<N+1; a++) {
		if(i < lvlstart[a]) break;
	}
	int n_level = a-1;
	int leftfill = i - lvlstart[n_level];
	char result[3];
	for(int j=n_level;j>=0;j--)
	{
		int c = 1;
		for(int b = 0; b < j; b ++)
		{
			c *= candidatesns[b];
		}
		int leveli = (int)(leftfill / c);
		char tc = candidates[j][leveli];
		printf("level%d:%c\n", j, tc);
		result[n_level - j] = tc;
		leftfill = leftfill % c;
	}
	for(int j=0;j<=n_level;j++)
	{
		printf("%c", result[j]);
	}
	return 1;
}

int main(int argc, char const *argv[])
{

	test2();
	Client fcp;
	Client fc;
	Client scp;
	Client sc;
	fcp.i = 1;
	fcp.next = NULL;
	fc.i = 2;
	fc.next = NULL;
	scp.i = 3;
	scp.next = NULL;
	sc.i = 4;
	sc.next = NULL;


	fcp.next = &fc;
	scp.next = &sc;
	fc.next = &scp;

	/*fcp.next = &fc;*/
	/*fc.next = &sc;*/

	Client *fcpa = &fcp;
	Client *fca = &fc;
	Client *scpa = &scp;
	Client *sca = &sc;

	Client *tmp;
	Client **pp1;
	Client **pp2;
	pp1 = &(fcpa->next);
	pp2 = &(scpa->next);
	(*pp1)->i;
	/*printf("%d", (*pp1)->i);*/
	tmp = *pp1;
	*pp1 = *pp2;
	*pp2 = tmp;

	pp1 = &(fca->next);
	pp2 = &(sca->next);
	tmp = *pp1;
	*pp1 = *pp2;
	*pp2 = tmp;

	Client head;
	head.i = 0;
	head.next = fcpa;
	Client *tmp1 = &head;
	while (tmp1) {
		printf("%d", tmp1->i);
		tmp1 = tmp1->next;
	}

	printf("\n");
	

	double coln = sqrt(4);
	printf("%d",coln == (int)coln);
	/*int arr1[1][7] = {{1,1,1,1,1,1,1}};*/
	int arr1[1][7];
	memset(arr1, 0, 7 * sizeof(int));
	int i;
	int j;
	for(i=0;i<7;i++){
		for(j=0;j<1;j++)
		{
			arr1[i][j] = 1;
		}
	}
	printf("arr1 %d", arr1[0][0]);
	int result[2];
	/*right(arr1,1, 7, 0, 0, result);*/
	int m = *((int*)arr1 + 1*0 + 1);
	printf("m2:%d",m);
	printf("m3:%d", (int)**(arr1+1));
	printf("m3:%d", arr1[0][0]);
	int g = mmax(4,1,2,3,4);
	printf("aaaa: %d\n", g);
	test();
	readstruct2();

    int aaa[6] = {1,2,3,4,5,6};
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
    printf("tnow=%lu\n",tnow - 1);   // 输出整数表示的时间

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
