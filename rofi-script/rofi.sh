#! /bin/bash

# 定义源目录和目标目录
SOURCE_DIR="/home/beyond/software/bin/rofi-script"
TARGET_DIR="/tmp/rofi-script"
LINK_NAME="/home/beyond/software/bin/rofi-script_"
DIRTY_FLAG="/tmp/rofi-script.dirty"

# 检查软链接是否存在
if [ -d "$TARGET_DIR" ]; then
    echo "Link $LINK_NAME already exists. Checking for dirty flag..."
    
    # 检查是否存在 .dirty 文件
    if [ -d "$TARGET_DIR" ]; then
        echo "$DIRTY_FLAG exists. Syncing files..."
        rsync -av --progress "$SOURCE_DIR/" "$TARGET_DIR/"
        # 同步完成后，您可以选择删除 .dirty 文件
        rm "$DIRTY_FLAG"
        echo "Files synced from $SOURCE_DIR to $TARGET_DIR due to dirty flag."
    else
        echo "No dirty flag found. Executing..."
    fi
else
    # 检查目标目录是否存在
    if [ ! -d "$TARGET_DIR" ]; then
        echo "Directory $TARGET_DIR does not exist. Syncing files..."
        # 使用 rsync 同步文件
        rsync -av --progress "$SOURCE_DIR/" "$TARGET_DIR/"
        # 同步完成后，您可以选择删除 .dirty 文件
        rm "$DIRTY_FLAG"
        # 创建软链接
        ln -s "$TARGET_DIR" "$LINK_NAME"
        echo "Soft link $LINK_NAME created, pointing to $TARGET_DIR."
    else
        echo "Directory $TARGET_DIR already exists. Checking for changes..."

        # 检查是否存在 .dirty 文件
        if [ -f "$DIRTY_FLAG" ]; then
            echo "$DIRTY_FLAG exists. Syncing files..."
            rsync -av --progress "$SOURCE_DIR/" "$TARGET_DIR/"
            # 同步完成后，您可以选择删除 .dirty 文件
            rm "$DIRTY_FLAG"
            echo "Files synced from $SOURCE_DIR to $TARGET_DIR due to dirty flag."
        else
            echo "$DIRTY_FLAG not exists. Executing..."
        fi
    fi
fi

# 检查操作系统
if [[ "$(uname -a)" == *"arch"* ]]; then
  echo "当前操作系统是 Arch Linux"
  source /home/beyond/venv-common/bin/activate
fi

rofi -show fb -modes "fb:/home/beyond/software/bin/rofi-script_/_rofi.sh" -matching fuzzy
