#!/bin/bash
browserdir=$HOME/software/bin/chrominum/chrome-linux
browsercmd=$browserdir/chrome
echo 'export GOOGLE_API_KEY="no"
export GOOGLE_DEFAULT_CLIENT_ID="no"
export GOOGLE_DEFAULT_CLIENT_SECRET="no"
'$browsercmd' --user-data-dir='$browserdir' --new-window "$(printf $1 $(xclip -selection primary -o))"' > ~/software/browserclip.sh