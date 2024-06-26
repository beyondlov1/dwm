## DWM
这是我编译的`dwm`，补丁完美无冲突。

默认分支`master`并没有透明效果，如果想要透明效果可使用`alpha`分支。

## 补丁列表

- [dwm-autostart-20161205-bb3bd6f.diff](./patch/dwm-autostart-20161205-bb3bd6f.diff)
- [dwm-doublepressquit-6.3.diff](./patch/dwm-doublepressquit-6.3.diff)
- [dwm-fancybar-20200423-ed3ab6b.diff](./patch/dwm-fancybar-20200423-ed3ab6b.diff)
- [dwm-fullgaps-toggle-20200830.diff](./patch/dwm-fullgaps-toggle-20200830.diff)
- [dwm-gaplessgrid-20160731-56a31dc.diff](./patch/dwm-gaplessgrid-20160731-56a31dc.diff)
- [dwm-hide_vacant_tags-6.3.diff](./patch/dwm-hide_vacant_tags-6.3.diff)
- [dwm-launchers-20200527-f09418b.diff](./patch/dwm-launchers-20200527-f09418b.diff)
- [dwm-pertag-perseltag-6.2.diff](./patch/dwm-pertag-perseltag-6.2.diff)
- [dwm-restartsig-20180523-6.2.diff](./patch/dwm-restartsig-20180523-6.2.diff)
- [dwm-rotatestack-20161021-ab9571b.diff](./patch/dwm-rotatestack-20161021-ab9571b.diff)
- [dwm-scratchpad-6.2.diff](./patch/dwm-scratchpad-6.2.diff)
- [dwm-statuscmd-20210405-67d76bd.diff](./patch/dwm-statuscmd-20210405-67d76bd.diff)
- [dwm-systray-6.3.diff](./patch/dwm-systray-6.3.diff)
- [dwm-viewontag-20210312-61bb8b2.diff](./patch/dwm-viewontag-20210312-61bb8b2.diff)

### rofi 需要支持 EWMH, 打了下面两个补丁
- [wmhtags](https://dwm.suckless.org/patches/ewmhtags)
- [focusconnetactive](https://dwm.suckless.org/patches/focusonnetactive/)


## 截图

![desktop.png](./desktop.png)

### ubuntu 
依赖:
```
sudo apt install libxft-dev libxinerama-dev libimlib2-dev
sudo apt-get install yajl-tools  libyajl-dev

少包到  
https://packages.ubuntu.com/search?mode=exactfilename&suite=focal&section=all&arch=any&keywords=X11%2Fextensions%2FXinerama.h&searchon=contents
查询
```
- sudo -E make clean install  ## -E 代表携带当前用户环境变量
  sudo cp dwm.desktop /usr/share/xsessions
- mkdir -p ~/.config/dwm/script && echo 'compton 
ibus-daemon -r -d -x
while true; do
        xsetroot -name "$(date)"
        sleep 1
done' >| ~/.config/dwm/script/init.sh && chmod +x ~/.config/dwm/script/init.sh 
- echo 'export _JAVA_AWT_WM_NONREPARENTING=1' >> ~/.profile
- sudo apt remove --purge fonts-noto-color-emoji
- restart

### TODO
[x] 状态栏时间更新
[] 状态栏有窗口的显示不同背景色
