// source file : win.c
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xatom.h>
#include <linux/input.h>  
#include <linux/uinput.h>  
#include <unistd.h>
#include <fcntl.h>  


void simulate_mouse_key(int fd, int keycode)  
{  
  struct input_event event;  
  memset(&event, 0, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_MSC;  
  event.code = 4;  
  event.value = 90001;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_KEY;  
  event.code = keycode;  
  event.value = 1;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_SYN;  
  event.code = SYN_REPORT;  
  event.value = 0;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_MSC;  
  event.code = 4;  
  event.value = 90001;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_KEY;  
  event.code = keycode;  
  event.value = 0;  
  write(fd, &event, sizeof(event));  

  gettimeofday(&event.time, NULL);  
  event.type = EV_SYN;  
  event.code = SYN_REPORT;  
  event.value = 0;  
  write(fd, &event, sizeof(event));  

}


int main(int argc, char *argv[]) {
  char *outDevicePath = "/dev/input/event7";
  int outDeviceFd = open(outDevicePath, O_RDWR);
   if (outDeviceFd == -1)
   {
      perror("Failed to open input device");
      return 1;
   }
  simulate_mouse_key(outDeviceFd, 272);

  return 0;
}
