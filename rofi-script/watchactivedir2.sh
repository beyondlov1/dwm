#! /bin/bash

# 用inotify监听最近活动的目录

# 文件夹列表
dirs=(
    "/home/beyond"
)

# 监视目录
# 将数组元素连接成一个字符串，用空格分隔
WATCH_DIR=$(IFS=" " ; echo "${dirs[*]}")
# WATCH_DIR="/home/beyond/tmp/a"

# 输出文件
OUTPUT_FILE="/tmp/active_dirs.txt"
# 临时文件
TEMP_FILE="/tmp/temp_active_dirs.txt"
# 最大行数限制
MAX_LINES=10  # 设置最大行数限制

# 清空临时文件
> "$TEMP_FILE"

exclude_pattern='(^|/)\.[^/]+($|/)|temp_active_dirs.txt|active_dirs.txt|sed.*|\.swp$'

# 使用 inotifywait 监视目录，包括文件和文件夹的创建、删除、修改
inotifywait -m -r -e create,delete,modify --exclude "$exclude_pattern" $WATCH_DIR | while read -r LINE; do
    # 提取事件类型和文件路径
    EVENT=$(echo "$LINE" | awk '{print $2}')
    FILE=$(echo "$LINE" | awk '{print $1}')
    
    # 如果事件是 "CREATE,ISDIR"
    if [[ "$EVENT" == "CREATE,ISDIR" ]]; then
        # 直接拼接文件夹名称
        FILE="$FILE$(echo "$LINE" | awk '{print $3}')"
    fi

    echo $LINE

    # 检查输出文件的行数并限制长度
    while [ $(wc -l < "$OUTPUT_FILE") -gt "$MAX_LINES" ]; do
        # 删除最旧的行
        sed -i '1d' "$OUTPUT_FILE"
    done
    
    # 检查文件路径是否已存在于输出文件中（精确匹配）
    if grep -Fxq "$FILE" "$OUTPUT_FILE"; then
        # 如果存在，先删除该行
        # sed -i "\|$FILE|d" "$OUTPUT_FILE"
        sed -i "/^$(echo "$FILE" | sed 's/[\/&]/\\&/g')$/d" "$OUTPUT_FILE"
    fi
    
    # 将新路径或已存在的路径追加到临时文件
    echo "$FILE" >> "$TEMP_FILE"
    
    # 读取临时文件并写入输出文件，保持先进先出
    cat "$TEMP_FILE" >> "$OUTPUT_FILE"
    > "$TEMP_FILE"  # 清空临时文件
    
done
