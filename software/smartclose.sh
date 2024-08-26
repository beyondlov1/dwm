#! /bin/bash
role=$(xprop -id $(xdotool getwindowfocus getwindowgeometry --shell | grep WINDOW | awk -F= '{print $2}') | grep WM_WINDOW_ROLE | awk -F\" '{print $2}')
event=$(cat /proc/bus/input/devices | grep -A10 Keyboard | grep event | awk -F"=" '{print $2}' | grep -Eo "event[0-9]+" | head -n 1)
if [[ "$role" == "browser" || "$role" == "browser-window" ]]; then
	xdotool key "Ctrl+w"
	# sudo simulate_key $event 2 29 17
else
	xdotool key "Super+F1"
	# sudo simulate_key $event 21 59
	# sudo simulate_key $event 21 56
	# sudo simulate_key $event 2 56 62
fi

