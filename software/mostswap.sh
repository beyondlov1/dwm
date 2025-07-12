#!/bin/bash
#script name: swap_check.sh
#author: wuhs
#version: v1
#description: 这是一个检查哪些进程使用了SWAP分区的脚本
do_swap () {
  SUM=0
  OVERALL=0
  #获取进程目录
  for DIR in `find /proc/ -maxdepth 1 -type d | egrep "^/proc/[0-9]"` ; do
        #获取进程PID
    PID=`echo $DIR | cut -d / -f 3`
    #获取进程名称
    PROGNAME=`ps -p $PID -o comm --no-headers`
    #获取进程的所有分区中SWAP值
    for SWAP in `grep Swap $DIR/smaps 2>/dev/null| awk '{ print $2 }'`
    do
          #进程swap值求和
      let SUM=$SUM+$SWAP
    done
    echo "PID=$PID,$SUM,$PROGNAME"
    #总共swap分区值求和
    let OVERALL=$OVERALL+$SUM
    SUM=0 
  done
  echo "Overall swap used: $OVERALL"
 }
do_swap > tmp.txt
#查询结果排序并截取top10
cat tmp.txt |awk -F, '{print $2,$1,$3}' | sort -n | tail -10
cat tmp.txt |tail -1
rm -rf tmp.txt
