#! /bin/bash
cwinid=$(xdotool getwindowfocus)
note=$(zenity --entry)
if [[ -n $note ]];then
	xprop -id $cwinid -f _NET_MY_NOTE 8u -set _NET_MY_NOTE $note
else
	xprop -id $cwinid -f _NET_MY_NOTE 8u -set _NET_MY_NOTE "___"
fi
