1）功能说明：
该模块是爱芯SDK包提供的视频解码单元link模式示例代码，方便客户快速理解和掌握视频解码相关接口的使用。
代码演示了如下功能：解码模块初始化、解码Send线程解析和发送码流、IVPS/VO模块初始化、VO offline模式
下获得YUV图像线程，各模块退出功能。
编译后得到可执行程序sample_vdec_ivps_vo, 位于/opt/bin目录，可用于VDEC link IVPS，IVPS link VO 功能验证。

使用示例：
举例一：查看help信息
/opt/bin/sample_vdec_ivps_vo  -h

H264协议：
./sample_vdec_ivps_vo -i /opt/data/vdec/1080p.h264 -T 96 -W 1920 -H 1088

jpeg:
./sample_vdec_ivps_vo -i /opt/data/vdec/1080P_qp9.jpg -T 26 -W 1920 -H 1088

参数说明：
-i:  输入码流路径。
-T：解码类型；26：jpegdec； 96：h264 vdec； 265: h265 vdec 。
-L：循环解码次数，默认为0，表示为无限次。
--res： 设置输出分辨率，默认典型分辨率是1920x1080。
-v:  (暂不支持) 选择显示设备的类型、分辨率及刷新率，如dsi0@1920x1080@60
-w : (暂不支持) vo输出YUV的帧数，默认值是1，此值只有在没有接显示屏，即-v没有选择的情况下有效，用于设备没有接入外设使用vo offline功能获取YUV图像。
-W: 宽度参数，用于内部参考帧buffer申请，默认8192。关注内存时请需要按照实际值配置。
-H: 高度参数，用于内部参考帧buffer申请，默认8192。关注内存时请需要按照实际值配置。
注意：由于sample中vo offline输出YUV getframe在一个线程中执行，如果w值大于实际帧数，线程不能退出，需要Ctrl +C退出。



3）注意事项：
（1）用户如果需要配置其他参数,如rawType,修改sample代码即可。
（2）本sample只支持一路视频流的显示,在MIPI双屏的情况下,dsi0显示视频,dsi1显示彩条。
（3）本sample默认固定使用VDEC VdChn 0 link ivps 0， ivps 0 link vo 0。


4）Sample Log 开关：
echo 18 7 > /proc/ax_proc/logctl         # 允许 DEBUG LOG 输出； 具体 LEVEL 参看 ax_global_type.h 中的枚举类型 AX_LOG_LEVEL_E

5）AX SDK Vdec Log开关：
echo 8 7 > /proc/ax_proc/logctl         # 允许 DEBUG LOG 输出； 具体 LEVEL 参看 ax_global_type.h 中的枚举类型 AX_LOG_LEVEL_E


echo target [file, console, null] > /proc/ax_proc/logctl      # file：LOG 保存到板端 /opt/data/AXSyslog/syslog/ 路径下； console：LOG 从当前终端输出

