1) 功能说明：

该模块是UVC（USB Video Class）示例代码，方便客户快速理解和掌握VIN模块,VENC模块等相关接口实现UVC camera功能。
sample_uvc可执行程序以及uvc驱动设置脚本uvc-gadget-composite.sh,位于/opt/bin/sample_uvc/目录，
dummy sensor的jpg格式图片文件，位于/opt/data/uvc/目录支持650A平台。

支持单个及两个os08a20 sensor：
输出分辨率为360p，540p，720p，1080p，4K，帧率为30的MJPG/H264/H265格式的视频流;
输出分辨率为360p@30fps，480p@30fps, 540p@20fps，720p@10fps，1080p@5fps YUY2格式的视频流。

支持单个及两个dummy sensor:
输出分辨率为720p,帧率为30的MJPG格式的视频流，通过读取固定JPG格式图片数据来充当sensor数据，非真实sensor数据；
输出分辨率360p帧率为30，分辨率480p帧率为30，分辨率540p帧率为20，分辨率720p帧率为10，分辨率1080p帧率为5的
YUY2格式的视频流，通过绘制色彩条图片数据来充当sensor数据，非真实sensor数据，故称为dummy sensor。

支持通过应用程序启动isp tuning，在PC端使用tuning tool进行IQ tuning，
调试相关的IQ参数，通过UVC camera查看实时效果图像，该图像为VIN模块CHN1或者CHN2的图像输出。

PC端调试UVC Camera可以使用PotPlayer播放器，该播放器支持视频录制，截图，旋转等。

2）使用示例：

准备好650A平台的硬件环境，上电后，可通过串口连接上650A平台。使用typeC连接线连接板子和PC。

注意事项:
对于650A Demo板，在使用一个sensor时，sensor接在MIPI RX0-3接口上;在使用两个sensor时，第一个sensor
接在MIPI RX0-3接口上，第二个sensor接在MIPI RX4-7接口上。

运行脚本设置UVC驱动，其中对于第四个参数是选择sensor类型，0: dummy sensor, 1: os08a20 sensor，输入
其他值则会报错；参数三为实际创建的uvc通道数目，注意实际使用到了几个camera，就需要开启几路uvc通道。
./opt/bin/sample_uvc/uvc-gadget-composite.sh start usb2 1 1

参数1: start or stop
参数2: usb2 or usb3 is supported
参数3: uvc device count, support 2 devices at maximum
参数4: 0: os08a20 sensor, 1: dummy sensor

在650A平台板端启动UVC程序
示例一：
./opt/bin/sample_uvc/sample_uvc -d -n 4 -y 0
其中-y选项选择sensor类型, 0:单个os08a20 sensor, 1:两个os08a20 sensor, 2:单个dummy sensor, 3:两个dummy sensor。
该命令选择了0:单个os08a20 sensor。默认VIN模块和VENC模块之间采用LINK的方式。

示例二：
./opt/bin/sample_uvc/sample_uvc -d -n 4 -y 0 -w 0
其中-y选项选择sensor类型, 0:单个os08a20 sensor, 1:两个os08a20 sensor, 2:单个dummy sensor, 3:两个dummy sensor。
该命令选择了0:单个os08a20 sensor。-w 0选项表示VIN模块和VENC模块之间采用非LINK的方式

示例三：
./opt/bin/sample_uvc/sample_uvc -d -n 4 -y 1 -p -w 1
其中-y选项选择sensor类型, 0:单个os08a20 sensor, 1:两个os08a20 sensor, 2:单个dummy sensor, 3:两个dummy sensor。
该命令选择了1:两个os08a20 sensor。 -p选项表示开启isp tuning。 -w 1选项表示VIN模块和VENC模块之间采用LINK的方式。

示例四：
./opt/bin/sample_uvc/sample_uvc -d -n 4 -y 2
其中-y选项选择sensor类型, 0:单个os08a20 sensor, 1:两个os08a20 sensor, 2:单个dummy sensor, 3:两个dummy sensor。
该命令选择了2: 单个dummy sensor。不加-i选项，默认图片是/opt/data/uvc/1280x720.jpg。

示例五：
./opt/bin/sample_uvc/sample_uvc -d -n 4 -y 2 -i /opt/data/uvc/1280x720.jpg
其中-y选项选择sensor类型, 0:单个os08a20 sensor, 1:两个os08a20 sensor, 2:单个dummy sensor, 3:两个dummy sensor。
该命令选择了2:单个dummy sensor。其中-i选项指定图片路径,图片分辨率为1280x720，在-y 2的条件下，不加-i选项，默认图片
是/opt/data/uvc/1280x720.jpg。

示例六：
./opt/bin/sample_uvc/sample_uvc -d -n 4 -y 3 -i /opt/data/uvc/1280x720.jpg
其中-y选项选择sensor类型, 0:单个os08a20 sensor, 1:两个os08a20 sensor, 2:单个dummy sensor, 3:两个dummy sensor。
该命令选择了3:两个dummy sensor。其中-i选项指定图片路径，图片分辨率为1280x720，在-y 3的条件下，不加-i选项，默认图片
是/opt/data/uvc/1280x720.jpg。

参数1 -d: 对于uvc device而言采用dummy camera类型，即uvc device stand alone类型无v4l2 camera。
参数2 -n: v4l2 buffer个数
参数3 -y: sensor类型
参数4 -p: 启动isp tuning标志
参数5 -w: VIN与VENC模块之间采用LINK的方式工作
参数6 -a: 使能aiisp：1，开启；0，关闭，默认开启AIISP

UVC camera播放器软件操作说明
在互联网上下载一个PotPlayer软件安装包，安装在PC上。在PC上打开PotPlayer视频播放器,Alt+D快捷键打开
摄像头选项卡，在设备一栏中选择UVC Camera或者USB Video Device（可尝试检索更新按钮更新设备状态）。

对于os08a20 sensor在格式一栏中选择:
MJPG/H264/H265（640x360，960x540，1280x720，1920x1080，3840x2160）格式；
YUY2(640x360, 640x480, 960x540, 1280x720, 1920x1080)格式；
然后点击打开设备按钮，即可打开UVC Camera或者USB Video Device。

对于dummy sensor在格式一栏中选择:
MJPG（1280x720）格式，
YUY2(640x360,640x480,960x540,1280x720,1920x1080)格式。
然后点击打开设备按钮，即可打开UVC Camera或者USB Video Device。

在Potplayer播放器主界面左上角，点击下拉菜单“PotPlayer ∨”，选择“视频”功能，在子菜单中根据需要选择
“图像处理”，“视频录制”，“图像截取”等功能，实现图像处理、视频录制、截图保存功能。