#! /bin/bash

# 要用python3执行, 以使得前边的venv生效
# python3 /home/beyond/software/bin/rofi-script_/rofi.py "$*"
# /home/beyond/software/bin/rofi-script_/dist/rofi/rofi "$*"

# 定义变量
ROFI_SCRIPT="/home/beyond/github/dwm/rofi-script/rofi.py"
python3 "$ROFI_SCRIPT" "$@"

