#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <string.h>  
#include <stdio.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>  
#include <linux/input.h>  
#include <linux/uinput.h>  
#include <stdio.h>  
#include <sys/time.h>  
#include <sys/types.h>  
#include <unistd.h>  
#include <stdlib.h>

void simulate_key(int fd,int kval)  
{  
    struct input_event event;  
    event.type = EV_KEY;  
    event.value = 1;  
    event.code = kval;  

    gettimeofday(&event.time,0);  
    write(fd,&event,sizeof(event)) ;  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_KEY;  
        event.code = kval;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
}  
void simulate_mouse(int fd)  
{  
    struct input_event event;  
        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_REL;  
        event.code = REL_X;  
        event.value = 10;  
        write(fd, &event, sizeof(event));  

        event.type = EV_REL;  
        event.code = REL_Y;  
        event.value = 10;  
        write(fd, &event, sizeof(event));  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
}

void simulate_multi_key2(int fd, int key1, int key2, int key3)
{  
        struct input_event event;  

     //先发送一个 CTRL 按下去的事件。  
        event.type = EV_KEY;  
        event.value = 1;  
        event.code = key1;  
        gettimeofday(&event.time,0);  
        write(fd,&event,sizeof(event)) ;  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

     //先发送一个 CTRL 按下去的事件。  
        event.type = EV_KEY;  
        event.value = 1;  
        event.code = key2;  
        gettimeofday(&event.time,0);  
        write(fd,&event,sizeof(event)) ;  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

     //先发送一个 SPACE 按下去的事件。  
        event.type = EV_KEY;  
        event.value = 1;  
        event.code = key3;  
        gettimeofday(&event.time,0);  
        write(fd,&event,sizeof(event)) ;  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
     //发送一个 释放 SPACE 的事件  
        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_KEY;  
        event.code = key3;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

     //发送一个 释放 CTRL 的事件  
        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_KEY;  
        event.code = key2;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

     //发送一个 释放 CTRL 的事件  
        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_KEY;  
        event.code = key1;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
}  

void simulate_multi_key(int fd, int key1, int key2)
{  
        struct input_event event;  

     //先发送一个 CTRL 按下去的事件。  
        memset(&event, 0, sizeof(event));  
        event.type = EV_KEY;  
        event.value = 1;  
        event.code = key1;  
        gettimeofday(&event.time,0);  
        write(fd,&event,sizeof(event)) ;  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

     //先发送一个 SPACE 按下去的事件。  
        memset(&event, 0, sizeof(event));  
        event.type = EV_KEY;  
        event.value = 1;  
        event.code = key2;  
        gettimeofday(&event.time,0);  
        write(fd,&event,sizeof(event)) ;  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

     //发送一个 释放 SPACE 的事件  
        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_KEY;  
        event.code = key2;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

     //发送一个 释放 CTRL 的事件  
        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_KEY;  
        event.code = key1;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
}  


void release_key(int fd, int key1)
{  
        struct input_event event;  

     //发送一个 释放 CTRL 的事件  
        memset(&event, 0, sizeof(event));  
        gettimeofday(&event.time, NULL);  
        event.type = EV_KEY;  
        event.code = key1;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
}  


void press_key(int fd, int key1)
{  
        struct input_event event;  

    //先发送一个 CTRL 按下去的事件。  
        memset(&event, 0, sizeof(event));  
        event.type = EV_KEY;  
        event.value = 1;  
        event.code = key1;  
        gettimeofday(&event.time,0);  
        write(fd,&event,sizeof(event)) ;  

        event.type = EV_SYN;  
        event.code = SYN_REPORT;  
        event.value = 0;  
        write(fd, &event, sizeof(event));  
}  


int main(int argc, const char *argv[])
{
   const char *inputDevicePath = "/dev/input/event5";
   
   /*const char *outDevicePath = "/dev/input/event3";*/
   char outDevicePath[64];
   strcpy(outDevicePath, "/dev/input/");
   strcat(outDevicePath, argv[1]);

   int inputDeviceFd = open(inputDevicePath, O_RDONLY);
   int outDeviceFd = open(outDevicePath, O_RDWR);

   if (inputDeviceFd == -1)
   {
      perror("Failed to open input device");
      return 1;
   }

   if(atoi(argv[2]) == 1){
      simulate_key(outDeviceFd, atoi(argv[argc-1]));
   }

   if(atoi(argv[2]) == 2){
      simulate_multi_key(outDeviceFd, atoi(argv[argc-2]),  atoi(argv[argc-1]));
   }

   if(atoi(argv[2]) == 3){
      simulate_multi_key2(outDeviceFd, atoi(argv[argc-3]),  atoi(argv[argc-2]),   atoi(argv[argc-1]));
   }

   if(atoi(argv[2]) == 11 ){
      release_key(outDeviceFd, atoi(argv[argc-3]));
      simulate_key(outDeviceFd,  atoi(argv[argc-2]));
      press_key(outDeviceFd, atoi(argv[argc-1]));
   }

   if(atoi(argv[2]) == 12 ){
      release_key(outDeviceFd, atoi(argv[argc-4]));
      simulate_multi_key(outDeviceFd, atoi(argv[argc-3]),  atoi(argv[argc-2]));
      press_key(outDeviceFd, atoi(argv[argc-1]));
   }

   if(atoi(argv[2]) == 13 ){
      release_key(outDeviceFd, atoi(argv[argc-5]));
      simulate_multi_key2(outDeviceFd, atoi(argv[argc-4]), atoi(argv[argc-3]),  atoi(argv[argc-2]));
      press_key(outDeviceFd, atoi(argv[argc-1]));
   }

   printf("%d",argc);
 
   close(inputDeviceFd);

   return 0;
}

