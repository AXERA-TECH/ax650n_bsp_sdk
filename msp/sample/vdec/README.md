1）功能说明：
该模块是爱芯SDK包提供的视频解码单元示例代码，方便客户快速理解和掌握视频解码相关接口的使用。
代码演示了如下功能：解码模块初始化、解码Send线程解析和发送码流、解码Get线程获取和保存YUV数据、解码模块去初始化。
编译后得到可执行程序sample_vdec,位于/opt/bin目录，可用于视频解码功能验证。

-i: 输入码流路径。
-c：解码通道数；默认为 1。
-T：解码类型；26：jpegdec； 96：h264 vdec； 265: h265 vdec。
-w：是否将解码后的YUV保存到文件， 0：不保存； 1：保存。
-L：循环解码次数，默认为 1次。
-W: 宽度参数，用于内部参考帧buffer申请，默认8192。关注内存时请需要按照实际值配置。
-H: 高度参数，用于内部参考帧buffer申请，默认8192。关注内存时请需要按照实际值配置。
-M: 发送码流模式；0：按NAL方式发送码流，每次发送以一个NAL为单位；
                1：帧发送模式，每次发送以一帧为单位；
                2: SLICE发送模式，每次发送一个SLICE（暂不支持）；
                3：流发送模式，每次发送不定长度码流（JPEG/MJPEG不支持）；
                4：兼容发送模式，一帧码流可分多次发送，当一帧码流发送完毕时 bEndOfFrame 需置为 AX_TRUE；
-s：发送码流模式为流发送模式有效，每次流发送长度，值大于0小于等于输入buffer长度有效，sample中buffer长度为3MBytes,该值默认值为1MBytes。

--VdChn： 设置输出通道号， AX650 VDEC 支持0~2， JDEC 仅支持0。
--res： 设置输出分辨率，默认典型分辨率是1920x1080。
--select 1：call AX_VDEC_SelectGrp（默认）；0：disable

--outFormat：输出YUV图像格式；0: YUV400, 3: NV12 （for 8-bit/10-bit stream）, 4: NV21（for 8-bit/10-bit stream）,
                       42:10bitP010, 40:10bitY/U/V 4 pixels in 5 bytes. default: 3

2）使用示例：
举例一：查看help信息
/opt/bin/sample_vdec  -h

举例二: 解码1080p jpeg，并将解码后的yuv保存到当前目录
/opt/bin/sample_vdec -i /opt/data/vdec/1080P_qp9.jpg -T 26 -w 1 --res=1920x1080 -W 1920 -H 1088

举例三：解码1080p h264，并将解码后的yuv保存到当前目录
/opt/bin/sample_vdec -i /opt/data/vdec/1080p.h264 -T 96 -w 1 --res=1920x1080 -W 1920 -H 1088

3）运行结果：
运行成功后，在当前目录应生成解码后的yuv数据，名称group0.yuv，用户可打开看实际效果。

4）Sample Log 开关：
echo 18 7 > /proc/ax_proc/logctl         # 允许 DEBUG LOG 输出； 具体 LEVEL 参看 ax_global_type.h 中的枚举类型 AX_LOG_LEVEL_E

5）AX SDK Vdec Log开关：
echo 8 7 > /proc/ax_proc/logctl         # 允许 DEBUG LOG 输出； 具体 LEVEL 参看 ax_global_type.h 中的枚举类型 AX_LOG_LEVEL_E


echo target [file, console, null] > /proc/ax_proc/logctl      # file：LOG 保存到板端 /opt/data/AXSyslog/syslog/ 路径下； console：LOG 从当前终端输出

