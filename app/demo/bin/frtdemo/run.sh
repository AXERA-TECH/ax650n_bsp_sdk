#!/bin/sh
cur_path=$(cd "$(dirname $0)";pwd)

process=frtdemo

pid=$(pidof ${process})
if [ $pid ]; then
  echo "${process} is already running, please check the process(pid: $pid) first."
  exit 1;
fi

if [ -e /tmp/core* ]; then
  echo "exist coredump file under path: /tmp, please deal with coredump file first."
  exit 1;
fi

if [ $# -lt 2 ] ; then
  echo "USAGE: $0 -p <ppl> -s <sensor type> [-n <scenario>] [-l <log level>] [-t <log target>]"
  echo " e.g.: $0 -p 0 -s 0                  -- Start with ITS and dual os08a20"
  echo " e.g.: $0 -p 1 -s 0 -l 2 -t 2        -- Start with IPC and dual os08a20, log level set to ERROR, log target set to APP"
  echo " e.g.: $0 -p 2 -s 5 -n 0             -- Start with Pano dual os04a10 and select scenario 0"
  echo " ---------------------------------------------------------------------------------------------------------------------------"
  echo " Command details:"
  echo "   p: pipeline index, indicates ppl load type, like ITS or IPC or Pano etc."
  echo "      0: ITS pipeline "
  echo "      1: IPC pipeline"
  echo "      2: Pano pipeline"
  echo "   s: sensor type."
  echo "      0: DUAL OS08A20 "
  echo "      1: Single OS08A20"
  echo "      2: DUAL OS08B10"
  echo "      3: Single OS08B10"
  echo "      4: Single SC910gs"
  echo "      5: Pano Dual OS04A10"
  echo "   n: scenario, indicates the scenario to load, which is always defined in config files."
  echo "      ITS:"
  echo "          0: default (DEFAULT: dual sensor in T3DNR+T2DNR+T2DNR mode)"
  echo "          1: Dual sensor in T3DNR+T2DNR+AI2DNR mode"
  echo "          2: Single sensor in (Dual3DNR<->T3DNR)+T2DNR+AI2DNR mode"
  echo "          3: Single sensor with flash mode"
  echo "          4: Dual sensor in 4k@30fps AICE<->AINR SDR/HDR mode without jenc"
  echo "          5: Dual sensor in 4k@50fps Dual3DNR<->T3DNR SDR mode without jenc"
  echo "      IPC:"
  echo "          0: default (Single sensor in 8M@30fps 1-4-4)"
  echo "      Pano:"
  echo "          0: default (Dual sensor in 4M@30fps 2-1-4-4)"
  echo "   l: log level, indicates the log level."
  echo "      CRITICAL = 1, ERROR = 2 (DEFAULT), WARN = 3, NOTICE = 4, INFO = 5, DEBUG = 6, DATA = 7"
  echo "   t: log target, indicates the log output targets."
  echo "      SYSLOG = 1, APPLOG = 2, STDOUT = 4 (DEFAULT) (Calculate the sum if multiple targets is required)"
  echo "   d: GNU debugger."
  exit 1;
fi

#set -e
cd $cur_path

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./lib

# Enable core dump
EnableCoreDump=1
# Run in background
RunInBackground=0

# Check whether start with gdb debug mode
if [[ $(expr match "$*" ".*-d.*") != 0 ]]
then
  debug="gdb --args"
else
  debug=""
fi

# Open core dump
if [ $EnableCoreDump == 1 ] ; then
  ulimit -c unlimited
  echo /opt/data/core-%e-%p-%t > /proc/sys/kernel/core_pattern
fi

md5=`md5sum ${process} | awk '{ print $1 }'`
echo "launching ${process}, md5: ${md5} ..."

# launch
if [ $RunInBackground == 1 ] ; then
  nohup $debug ./${process} "$*" &
else
  $debug ./${process} $*
fi
