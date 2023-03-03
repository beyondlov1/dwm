#!/bin/bash
browsercmd=ba
echo $browsercmd' --new-window "$(printf $1 $(xclip -selection primary -o))"' > ~/software/browserclip.sh