#!/usr/bin/env bash

EXPECTED_ARGS=3

if [ $# -ne $EXPECTED_ARGS ]
then
    echo 'Got' $# 'argument(s), expected' $EXPECTED_ARGS
    echo 'Usage' $0 'operand1 operation operand2'
    exit 1
fi

HAVE_MODULE=`grep qwalculator /proc/modules`

if [ "x$HAVE_MODULE" == "x" ];
then
    sudo insmod qwalculator.ko
    echo 'Module is loaded now'
else
    echo 'Module is already loaded'
fi

op_device_num=`grep calc_inp3 /proc/devices | awk '{print $1}'`
var1_device_num=`grep calc_inp1 /proc/devices | awk '{print $1}'`
var2_device_num=`grep calc_inp2 /proc/devices | awk '{print $1}'`
out_device_num=`grep calc_out /proc/devices | awk '{print $1}'`

echo 'Major number of the operation device:' $op_device_num
echo 'Major number of the first operand device:' $var1_device_num
echo 'Major number of the second operand device:' $var2_device_num
echo 'Major number of the result device:' $out_device_num

sudo mknod /dev/calc_inp1 c $var1_device_num 1
sudo chmod a+r+w /dev/calc_inp1

sudo mknod /dev/calc_inp2 c $var2_device_num 1
sudo chmod a+r+w /dev/calc_inp2

sudo mknod /dev/calc_inp3 c $op_device_num 1
sudo chmod a+r+w /dev/calc_inp3

sudo mknod /dev/calc_out c $out_device_num 1
sudo chmod a+r+w /dev/calc_out

echo "$1" > /dev/calc_inp1
echo "$2" > /dev/calc_inp3
echo "$3" > /dev/calc_inp2

echo 'Content of operand1:'
cat /dev/calc_inp1
echo
echo 'Content of operation:'
cat /dev/calc_inp3
echo
echo 'Content of operand2:'
cat /dev/calc_inp2
echo
echo 'Content of result:'
cat /dev/calc_out
echo

