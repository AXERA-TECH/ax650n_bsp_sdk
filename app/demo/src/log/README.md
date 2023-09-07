applog
=======

## 概述
1. 支持syslog、标准输出和App本地日志
2. 支持同时输出到syslog、标准输出和本地文件
3. 日志格式类似Android adb logcat
4. AppLog保存路径/var/log/xxx_n.log, 最多支持5个循环文件，每个文件最大1MB
5. syslog初始化在SDK AX_SYS_Init被调用，反初始化在SDK AX_SYS_DeInit被调用

## API
1. #include "AppLogApi.h"
2. AX_APP_Log_Init
   如果要输出多个文件，Target用|连接，比如:APP_LOG_ATTR_T.nTarget = APP_LOG_TARGET_SYSLOG | APP_LOG_TARGET_STDOUT;
3. LOG_X or LOG_M_X
4. AX_APP_Log_DeInit

Example:
    日期和时间       线程ID 类别 TAG       日志信息
03-26 12:06:35:390  37314 E: this is an error log.
03-26 12:06:35:390  37314 N tag2: this is a notice tag2 log.
03-26 12:06:35:390  37314 W tag4: this is a warning tag4 log.
03-26 12:06:35:390  37314 --> 100(0x00000064) Bytes
                          00000000h: A3 97 A2 55 53 BE F1 FC F9 79 6B 52 14 13 E9 E2  ...US....ykR....
                          00000010h: 2D 51 8E 1F 56 08 57 27 A7 05 D4 D0 52 82 77 75  -Q..V.W'....R.wu
                          00000020h: 1B 99 4A ED 58 3D 6A 52 36 D5 24 4A 68 8E AD 95  ..J.X=jR6.$Jh...
                          00000030h: 5F 3C 35 B5 C4 8C DD 6C 11 32 3D E2 B4 B4 59 CF  _<5....l.2=...Y.
                          00000040h: CE 23 3D 27 DF A7 F9 96 FC 1E E0 66 2C 0E 7B 8C  .#='.......f,.{.
                          00000050h: CA 30 42 8F BC 9F 7B CE D1 B8 B1 87 EC 8A D6 BB  .0B...{.........
                          00000060h: 2E 15 63 0E

