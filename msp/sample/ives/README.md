sample_ives

1）功能说明：
ives文件夹下面的代码, 是爱芯SDK包提供的示例参考代码, 方便客户快速的理解IVES整个模块的配置流程.
示例代码演示的是如下的功能：MD移动侦测和OD遮挡检测
 (1) sample/ives/md文件夹是MD移动侦测的示例参考代码
 (2) sample/ives/od文件夹是OD遮挡检测的示例参考代码
 (3) sample/ives/scd文件夹是SCD场景切换检测的示例参考代码


2）使用示例：
 (1) MD
      ./sample_ives -t 0
      sample将读取/opt/data/ives下的3幅图片模拟MD结果，其中：
           1920x1080_ref0.nv12.yuv是参考背景
           1920x1080_det1.nv12.yuv和1920x1080_det2.nv12.yuv为检测图像
 (2) OD
      ./sample_ives -t 1

 (3) SCD
      ./sample_ives -t 2 --ref=save_1280x720_1.nv12 --cur=save_1280x720_2.nv12 --width=1280 --height=720


3）示例运行结果：
 (1) MD
/opt/bin # sample_ives -t 0
IVES sample: V0.31.0_20220409003934 build: Apr  9 2022 01:11:33
MD (Chn 0, img: 1, elapsed time: 7 ms
IMAGE 1 MB THRS: 20 x 15 = 300
 1  0  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1
MD (Chn 0, img: 2, elapsed time: 7 ms
IMAGE 2 MB THRS: 20 x 15 = 300
 0  0  0  0  0  0  0  0  1  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0
 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  1  0  0

 (2) OD
/opt/bin # sample_ives -t 1
IVES sample: V0.31.0_20220409003934 build: Apr  9 2022 01:11:33
OD (Chn 0, img: 0, od: 0, elapsed time: 6 ms

 (3) SCD
/opt/bin # ./sample_ives -t 2 --ref=save_1280x720_bgr1.nv12 --cur=save_1280x720_bgr2.nv12 --width=1280 --height=720
IVES sample: V0.39.0 build: Jun 29 2022 20:54:33
SCD (Chn 1, img: 1, elapsed time: 172 ms
SCD (Chn 1, img: 2, elapsed time: 172 ms
Scene change detection result: unchanged

4）注意事项：
 (1) MD的区域配置不能超过图像宽高，区域必须被宏块整除，具体参考IVES的用户手册文档；
 (2) OD示例代码仅简单的示例API如何调用，没有配置环境LUX，具体如何从ISP获取AE LUX和配置LUX请咨询爱芯工程师或者参考IPCDemo中OD功能。
