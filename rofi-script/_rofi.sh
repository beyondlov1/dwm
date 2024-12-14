#! /bin/bash

# 要用python3执行, 以使得前边的venv生效
# python3 /home/beyond/software/bin/rofi-script_/rofi.py "$*"
# /home/beyond/software/bin/rofi-script_/dist/rofi/rofi "$*"

# 定义变量
ROFI_SCRIPT="/home/beyond/software/bin/rofi-script_/rofi.py"
ROFI_EXEC="/home/beyond/software/bin/rofi-script_/dist/rofi"

# 检查 rofi 文件是否存在
if [ -f "$ROFI_EXEC" ]; then
    # 如果存在，执行 rofi
    "$ROFI_EXEC" "$@"
else
    # 如果不存在，执行 rofi.py
    python3 "$ROFI_SCRIPT" "$@"
fi

