1）功能说明：
该模块是爱芯SDK包提供的解码、IVPS、编码性能消耗的demo示例代码，方便客户快速理解和掌握视频解码相关接口的使用。
代码演示了如下功能：解码模块初始化、解码Send线程解析和发送码流、IVPS/VENC模块初始化、各模块退出功能。
编译后得到可执行程序sample_vdec_ivps_venc, 位于/opt/bin目录，可用于VDEC link IVPS，IVPS link venc 功能验证。
该模块解码仅在帧模式下支持，其余模式未做支持。

2) 参数说明：
-i:  输入码流路径。
-T：解码类型；26：jpegdec； 96：h264 vdec； 265: h265 vdec 。
-L：循环解码次数，默认为0，表示为无限次。
--res： 设置输出分辨率，默认典型分辨率是1920x1080。用于配置vdec、ivps、venc宽高属性，配置是必须的。
-W: 宽度参数，用于内部参考帧buffer申请，默认8192。关注内存时请需要按照实际值配置。
-H: 高度参数，用于内部参考帧buffer申请，默认8192。关注内存时请需要按照实际值配置。
-c：指定通道数
--enDisplayMode：0=预览模式，1=反压模式(VENC不支持反压模式，故该模块仅支持预览模式)

使用示例：
查看help信息:
/opt/bin/sample_vdec_ivps_venc

H264协议：
./sample_vdec_ivps_venc -i /opt/data/vdec/1080p.h264 --res=1920x1080 -W 1920 -H 1088 -L 100 -c 1 --enDisplayMode 0 -T 96

jpeg:
./sample_vdec_ivps_venc -i /opt/data/vdec/1080P_qp9.jpg --res=1920x1080 -W 1920 -H 1088 -L 100 -c 1 --enDisplayMode 0 -T 26

3）注意事项：
(1）用户如果需要配置其他参数,如rawType,修改sample代码即可。
(2）本sample默认固定使用vdec chn 0 link ivps 0， ivps 0 link venc 0。

4）Sample Log 开关：
echo 18 7 > /proc/ax_proc/logctl         # 允许 DEBUG LOG 输出； 具体 LEVEL 参看 ax_global_type.h 中的枚举类型 AX_LOG_LEVEL_E

5）AX SDK Vdec Log开关：
echo 8 7 > /proc/ax_proc/logctl         # 允许 DEBUG LOG 输出； 具体 LEVEL 参看 ax_global_type.h 中的枚举类型 AX_LOG_LEVEL_E

echo target [file, console, null] > /proc/ax_proc/logctl      # file：LOG 保存到板端 /opt/data/AXSyslog/syslog/ 路径下； console：LOG 从当前终端输出

