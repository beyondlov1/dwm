#! /bin/bash
role=$(xprop -id $(xdotool getwindowfocus getwindowgeometry --shell | grep WINDOW | awk -F= '{print $2}') | grep WM_WINDOW_ROLE | awk -F\" '{print $2}')
event=$(cat /proc/bus/input/devices | grep -A10 Keyboard | grep event | awk -F"=" '{print $2}' | grep -Eo "event[0-9]+" | head -n 1)
if [[ "$role" == "browser" ]]; then
	sleep 0.1 && sudo simulate_key $event 21 60
	sudo simulate_key $event 2 29 19
else
	sleep 0.1 && sudo simulate_key $event 21 60
	sudo simulate_key $event 1 60
fi

