#! /bin/bash

# 检查操作系统
if [[ "$(uname -a)" == *"arch"* ]]; then
  echo "当前操作系统是 Arch Linux"
  source /home/beyond/venv-common/bin/activate
fi

rofi -show fb -modes "fb:/home/beyond/software/bin/rofi-script/_rofi.sh" -matching fuzzy
# rofi -show fb -modes "fb:/home/beyond/github/dwm/rofi-script/_rofi.sh" -matching fuzzy
