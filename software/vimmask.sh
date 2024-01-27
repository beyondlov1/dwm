#!/bin/bash
check=$(wmctrl -l | grep  N/A)
if [ x"$check" == x"" ];then
  sudo vimmask $(cat /proc/bus/input/devices | grep -A10 Mouse | grep event | awk -F"=" '{print $2}' | grep -Eo "event[0-9]+" | head -n 1)
fi
