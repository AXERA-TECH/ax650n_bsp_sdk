#FRTDemo (前端产品形态demo)

## 如何执行？
### 执行ITS Pipeline
1. cd /opt/bin/FRTDemo
2. （可选）vi config/its/ppl.json 配置pipeline上前后两级模块对应关系。
3. （可选）vi config/its/ivps.json 配置IVPS模块参数。
4. （可选）vi config/its/encoder.json 配置编码通道参数。
5. （可选）vi config/its/options.ini 配置运行时各功能参数，包括RTSP/WEB缓存帧buff大小，OSD/板端录像等功能开关。[参数说明](#配置参数)
6. 执行./run.sh -p 0 -s 1 -n 2 [参数说明](#启动参数)
>   添加-d用于gdb调试，内部使用了SIGUSR2信号作为定时器，因此gdb调试时输入**handle SIGUSR2 nostop noprint**忽略SIGUSR2信号。

### 执行IPC Pipeline (待实现)
1. cd /opt/bin/FRTDemo
2. 执行./run.sh -p 1 -s 1 [参数说明](#启动参数)

## 如何连接Pano Pipelie的sensor
1. 选择型号为"ADP_RX_DPHY_4X4LANE_V1_0,2022/06/17"的ADP_RX_DPHY板，对于Pano Dual os04a10 Sensor，将两个os04a10 sensor分别连接到RX4/5, RX6/7上。
2. avs的相关配置参数文件存放在/param/avs/os04a10目录中。

## 如何编译？
1. cd app/demo/src/ppl/frt
2. make p=xxx clean
3. make p=xxx
4. make p=xxx install
> p=xxx 指定编译项目名，示例：make p=AX650_emmc

## 如何预览？
1. RTSP流预览：可通过第三方工具，比如VLC，输入RTSP流地址（参照终端打印输出：“Play the stream using url: <<<<< rtsp://IP:8554/axstream0 >>>>>”）进行预览。
2. WEB预览：可通过Chrome打开网页（参照终端打印输出：“Preview the video using URL: <<<<< http://IP:8080 >>>>>”）进行预览。

# <a href="#启动参数">启动参数</a>
p: pipeline index, indicates ppl load type, like ITS or IPC or Pano etc.
   0: ITS pipeline (DEFAULT)
   1: IPC pipeline
   2: Pano pipeline
s: sensor type.
   0: DUAL OS08A20 (DEFAULT)
   1: Single OS08A20
   5: Pano Dual 0SO4A10
n: scenario, indicates the scenario to load, which is always defined in config files.
   0: default (DEFAULT: dual sensor in T3DNR+T2DNR+T2DNR mode)"
   1: Dual sensor in T3DNR+T2DNR+AI2DNR mode"
   2: Single sensorin (Dual3DNR<->T3DNR)+T2DNR+AI2DNR mode"
   4: Dual sensor in 4k@30fps AICE<->AINR SDR/HDR mode without jenc"
   5: Dual sensor in 4k@50fps Dual3DNR<->T3DNR SDR mode without jenc"
l: log level, indicates the log level.
   CRITICAL = 1, ERROR = 2 (DEFAULT), WARN = 3, NOTICE = 4, INFO = 5, DEBUG = 6, DATA = 7
t: log target, indicates the log output targets.
   SYSLOG = 1, APPLOG = 2, STDOUT = 4 (DEFAULT) (Calculate the sum if multiple targets is required)
d: start with gdb for debugging, value **NOT REQUIRED**
u: testsuite type.
   0: Dual default
   1: Single default

# <a href="#配置参数">配置参数</a>

|   #   |          参数         |    参数范围   |       说明                        |
| ----- | --------------------- | ------------ | ---------------------------------|
|   1   | RTSPMaxFrmSize        | [0 - 8000000]| 单位：B                           |
|   2   | WebVencFrmSizeRatio   |              | Web缓存帧相对YUV的size比例         |
|   3   | WebJencFrmSizeRatio   |              | Web缓存帧相对YUV的size比例         |
|   4   | PrintFPS              |              | 0：不打印 1:打印                   |
|   5   | EnableOSD             |              | 0：关闭OSD 1:开启OSD               |
|   6   | EnableMp4Record       |              | 0：关闭板端录像 1:开启板端录像      |
|   7   | MP4RecordSavedPath    |              | 板端录像保存路径                   |
