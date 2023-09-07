sample_rtc /dev/rtc0 power_on
定时15s打印 Test complete,失败显示Test failed
sample_rtc /dev/rtc0 shut_down
定时15s打印，时间到机器关闭
sample_rtc /dev/rtc0 soft_restart
定时15s打印，时间到机器重启
sample_rtc /dev/rtc0 hard_restart
定时15s打印，时间到机器重启
sample_rtc /dev/input/event0
按下按键1s，松开可以看到按键上报
