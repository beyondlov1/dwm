#! /bin/bash
role=$(xprop -id $(xdotool getwindowfocus getwindowgeometry --shell | grep WINDOW | awk -F= '{print $2}') | grep WM_WINDOW_ROLE | awk -F\" '{print $2}')
# class=$(hyprctl -j activewindow | jq -r ".class")
if [[ "$role" == "browser" || "$class" == "firefox" ]]; then
	xdotool key "Ctrl+Tab"
	# sudo simulate_key $(cat /proc/bus/input/devices | grep -A10 Keyboard | grep event | awk -F"=" '{print $2}' | grep -Eo "event[0-9]+" | head -n 1) 12 56 29 15 56
fi
