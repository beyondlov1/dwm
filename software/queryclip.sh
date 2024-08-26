pid_path=/tmp/simplepage.tmp.pid
if [ -f $pid_path ]; then
	dbus-send --session --type=method_call --print-reply --dest=com.beyond.simplepage /com/beyond/simplepage com.beyond.simplepage.interface.loadUrl string:"$(printf $1 "$(xclip -selection primary -o)")"
else
echo 1 > $pid_path && /home/beyond/software/simplepage "$(printf $1 "$(xclip -selection primary -o)")" $2 $3 && rm $pid_path
fi
