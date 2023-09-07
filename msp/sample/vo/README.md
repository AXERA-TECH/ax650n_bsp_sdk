
1）功能说明：
vo文件夹下面的代码, 是爱芯SDK包提供的示例参考代码, 方便客户快速的理解VO整个模块的配置流程。
示例代码演示的是如下的功能：VO模块初始化、Layer/Chn的使用、以及图像的显示。/opt/etc/vo.ini
用于不能演示功的配置集。

2）使用示例：
A. 通过HDMI输出分辨率为1920x1080 60Hz的彩条, 运行命令如下：
   sample_vo -d 0

   -d: layer+display，参数0是指/opt/etc/vo.ini配置文件中[case0]下的配置参数，目前vo.ini [case0]
   配置下的参数指定dev0上 hdmi 1920x1080@60显示，详见vo.ini文件。

B. 只layer处理，运行命令如下：
   sample_vo -l 1

   -l：只layer处理，不显示。参数1是指/opt/etc/vo.ini配置文件中[case1]下的配置参数，目前vo.ini [case1]配置
   下的参数指定layer 36通道测试

C. VO连续播放显示，使用vo.ini配置文件中case10的配置，加载/mnt/video/1920x1080_cheliangdaolu2_30fps_300f_NV12.yuv
   连续文件进行播放，运行命令如下：
   sample_vo -p 10

   -p：播放显示，参数10是指/opt/etc/vo.ini配置文件中[case10]下的配置参数，[case10]配置下的参数
   chn_file_name = /mnt/video/1920x1080_cheliangdaolu2_30fps_300f_NV12.yuv 指定加载的播放文件。

D. 枚举显示设备支持的分辨率，运行命令如下：
   sample_vo -e 0

   -e：指定要枚举的设备，参数0指定需枚举的设备，支持的设备号有0、1、2。

E. HDMI口热插拔事件响应测试，运行命令如下：
   sample_vo -g

   监听HDMI口热插拔事件，收到事件打印输出信息。