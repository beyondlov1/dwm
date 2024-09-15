# xclip -selection clipboard -t image/png -o > /tmp/image.png
# # curl -X POST -F 'file=@/tmp/image.png' http://localhost:8000
# curl -X POST --data-binary @/tmp/image.png http://localhost:8000

flameshot gui && xclip -selection clipboard -t image/png -o | curl -X POST --data-binary @- http://localhost:8000 


