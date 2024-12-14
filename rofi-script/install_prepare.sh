#! /bin/bash

append_if_not_exists() {
    local line_to_add="$1"
    local file_path="$2"
    
    # 检查文件是否存在，不存在则创建
    if [ ! -f "$file_path" ]; then
        touch "$file_path"
        echo "$line_to_add" >> "$file_path"
        echo "File created and line added: $file_path"
        return 0
    fi
    
    # 使用精确匹配检查行是否存在
    if ! grep -qxF "$line_to_add" "$file_path"; then
        echo "$line_to_add" >> "$file_path"
        echo "Line added to $file_path"
    else
        echo "Line already exists in $file_path"
        return 1
    fi
}

append_if_not_exists "pyenv" ".gitignore"
append_if_not_exists "build" ".gitignore"
append_if_not_exists "dist" ".gitignore"
append_if_not_exists "__pycache__" ".gitignore"

if [[ ! -d "pyenv" ]]; then
  python3 -m venv pyenv
fi

pyenv/bin/pip3 install -i https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple -r requirements.txt
pyenv/bin/pip3 install -i https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple pyinstaller
