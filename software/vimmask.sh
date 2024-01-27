#!/bin/bash
check=$(wmctrl -l | grep  N/A)
if [ x"$check" == x"" ];then
  sudo vimmask event7
fi
