export GOOGLE_API_KEY="no"
export GOOGLE_DEFAULT_CLIENT_ID="no"
export GOOGLE_DEFAULT_CLIENT_SECRET="no"
/home/beyond/software/bin/chrominum/chrome-linux/chrome --user-data-dir=/home/beyond/software/bin/chrominum/chrome-linux/my-user-dir "$(printf $1 "$(xclip -selection primary -o)")"
