#! /bin/bash

if [[ "$(uname -a)" == *"arch"* ]]; then
  echo "当前操作系统是 Arch Linux"
  # source /home/beyond/venv-common/bin/activate
  rm -rf ${HOME}/software/bin/rofi-script/dist
  HERE=$(pwd)
  su - beyond -c "source $HERE/rofi-script/pyenv/bin/activate && pyinstaller -F $HERE/rofi-script/rofi.py --noconfirm --distpath $HERE/rofi-script/dist"
  cp -rf rofi-script/dist ${HOME}/software/bin/rofi-script

cat > $HOME/software/roficontext <<EOF
ROFI_DATA="/context" /usr/local/bin/rofi -pid /tmp/rofi-context.pid -normal-window -cache-dir /tmp/roficache -show fb -modes "fb:/home/beyond/github/dwm/rofi-script/_rofi.sh" -matching fuzzy -hold-window > /dev/null 2>&1 &
EOF
chmod +x $HOME/software/roficontext

else
  rm -rf ${HOME}/software/bin/rofi-script/dist
  HERE=$(pwd)
  su - beyond -c "source $HERE/rofi-script/pyenv/bin/activate && pyinstaller -F $HERE/rofi-script/rofi.py --noconfirm --distpath $HERE/rofi-script/dist"
  cp -rf rofi-script/dist ${HOME}/software/bin/rofi-script

cat > $HOME/software/roficontext <<EOF
ROFI_DATA="/context" /usr/local/bin/rofi -pid /tmp/rofi-context.pid -normal-window -cache-dir /tmp/roficache -show fb -modes "fb:/home/beyond/github/dwm/rofi-script/_rofi.sh" -matching fuzzy -hold-window > /dev/null 2>&1 &
EOF
chmod +x $HOME/software/roficontext

fi


