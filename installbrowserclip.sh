#!/bin/bash
browsercmd=google-chrome
echo $browsercmd' --new-window "$(printf $1 $(xclip -selection primary -o))"' > ~/software/browserclip.sh