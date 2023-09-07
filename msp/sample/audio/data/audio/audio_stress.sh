#!/bin/sh

#echo -n "should set cpu usage:"
#read cpu_usage
#if test $cpu_usage -gt 100
#then echo "input error!"
#elif test $cpu_usage  -le 0
#then echo "input error!"

#fi
#echo -n "Should set cpu number (default max):"
#read cpu_number
#echo -n "Set the CMM memory size that needs to be consumed in units such as 128MB (default 0): "
#read cmm_mem_size
#echo -n "Set the consumption system memory size with units such as 128MB (default 0): "
#read sys_mem_size
#echo -n "You need to set the memory release time unit in milliseconds (default 0):"
#read mem_stir_sleep
#echo $cpu_usage  $cpu_number $cmm_mem_size $sys_mem_size $mem_stir_sleep


pkill -9 lookbusy

#$cur_path/lookbusy -c $cpu_usage  -n $cpu_number -k $cmm_mem_size -M $mem_stir_sleep -m $sys_mem_size &
/opt/bin/lookbusy -k 500M -c 85 -n 8 -M 1000 &

# limit bandwidth
pkill -9 ct_dsp
/opt/data/dsp/ct_dsp  /opt/data/dsp/itcm.bin /opt/data/dsp/sram.bin r &

##限制带宽
#DDR  rd: sample_max*bitwidth_max*channel*6process + 200M(OTHERS)  wr=rd  (Duplex mode). 800M is from tests.
echo 800 > /proc/ax_proc/bw_limit/ddr_port0/limiter_val_rd
echo 800 > /proc/ax_proc/bw_limit/ddr_port0/limiter_val_wr

echo "success !"

#input is quiker than decode_output,so input_num is larger. 3000 and 800 are from tests. A test costs about 600s.
/opt/bin/sample_audio ai_aenc -D 0 -d 2 -G 30000 -r 48000 -p 480 --aec-mode 2 --routing-mode 0 --ns 1 --ag-level 2 --agc 1 --target-level -3 --resample 1 --resrate 48000 --layout 1 -e opus --aenc-chns 1 &
/opt/bin/sample_audio adec_ao -D 0 -d 3 -L 120 -r 48000  --ns 1 --ag-level 2 --agc 1 --target-level -3  -e g711a -i /opt/data/audio/audio_test.g711a

pkill -9 ct_dsp
pkill -9 lookbusy

echo 0 > /proc/ax_proc/bw_limit/ddr_port0/limiter_val_rd
echo 0 > /proc/ax_proc/bw_limit/ddr_port0/limiter_val_wr