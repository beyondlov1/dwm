kill $(xprop | grep _NET_WM_PID | awk '{print $3}')
