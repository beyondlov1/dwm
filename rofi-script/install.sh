#! /bin/bash

if [[ "$(uname -a)" == *"arch"* ]]; then
  echo "当前操作系统是 Arch Linux"
  # source /home/beyond/venv-common/bin/activate
  rm -rf ${HOME}/software/bin/rofi-script/dist
  HERE=$(pwd)
  su - beyond -c "source /home/beyond/venv-common/bin/activate && pyinstaller -D $HERE/rofi-script/rofi.py --noconfirm --distpath $HERE/rofi-script/dist"
  cp -rf rofi-script/dist ${HOME}/software/bin/rofi-script
else
  rm -rf ${HOME}/software/bin/rofi-script/dist
  HERE=$(pwd)
  su - beyond -c "source /home/beyond/venv-common/bin/activate && pyinstaller -D $HERE/rofi-script/rofi.py --noconfirm --distpath $HERE/rofi-script/dist"
  cp -rf rofi-script/dist ${HOME}/software/bin/rofi-script
fi


