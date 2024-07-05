#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/timeb.h>
#include <time.h>
#include <stdarg.h>

#define LENGTH(X) (sizeof X / sizeof X[0])

// 查看pid下是否有正在运行的子进程
int test3(){
	char cname[256+8] = "";
	char cmd[256];
	sprintf(cmd, "pstree %lu", 12382l);
	FILE *fp;
	char respbuf[1024];
	if ((fp = popen(cmd, "r"))){
		fgets(respbuf, sizeof(respbuf), fp);
		pclose(fp);
		char runningprogramname[256] = "";
  	sscanf(respbuf,"st---zsh---%s%*s", runningprogramname);
  	printf("%s",respbuf);
  	if(strcmp(runningprogramname, "") != 0)
			strcpy(cname, "*");
	}
	printf("%s", cname);
	return 0;
}

int main(int argc, char const *argv[])
{
	test3();
	return 0;
}
