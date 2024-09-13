# AX-Samples-AX650

`NPU Samples` 提供运行 `Pulsar2` 编译生成的 `axmodel` 模型的 example 代码，是演示 `AX650A/AX650N` 智能算法模型运行的必要示例。

已验证硬件平台

- AX650A
- AX650N

## 1. 示例分类

### 1.1 NPU 模型推理：

- 分类模型

  - MobileNetv1
  - MobileNetv2
  - Others

- 检测模型

  - YOLOv5s

## 2. 运行示例

### 2.1 运行准备

以 AX650A DEMO 开发板为例，无需其他准备动作，运行所需的模型和图片文件已经存在于 `/opt/data/npu` 目录下，可执行 sample 已经存在于 `/opt/bin/` 目录下。

- 将 **Pulsar2** 生成的 **axmodel** 模型拷贝到  `/opt/bin` 路径下；
- 将测试图片拷贝到 `/opt/bin` 路径下。

```
root@ax650:~# tree /opt/data/npu/
/opt/data/npu/
|-- images
|   |-- cat.jpg
|   `-- dog.jpg
`-- models
    |-- mobilenetv2.axmodel
    `-- yolov5s.axmodel

root@ax650:~# ls /opt/bin/sample_npu_*
/opt/bin/sample_npu_classification  /opt/bin/sample_npu_yolov5s
```

### 2.2 分类模型

下面演示运行分类模型，该 sample 的代码在 SDK 的 `msp/sample/npu/sample_classification` 文件夹下：

```bash
root@ax650:~# /opt/bin/sample_npu_classification -m /opt/data/npu/models/mobilenetv2.axmodel -i /opt/data/npu/images/cat.jpg  -r 100
--------------------------------------
model file : /opt/data/npu/models/mobilenetv2.axmodel
image file : /opt/data/npu/images/cat.jpg
img_h, img_w : 224 224
--------------------------------------
Engine creating handle is done.
Engine creating context is done.
Engine get io info is done.
Engine alloc io is done.
Engine push input is done.
--------------------------------------
topk cost time:0.07 ms
9.5094, 285
9.3773, 282
9.2452, 281
8.5849, 283
7.6603, 287
--------------------------------------
Repeat 100 times, avg time 0.72 ms, max_time 0.73 ms, min_time 0.72 ms
--------------------------------------
```

### 2.3 检测模型

下面演示运行检测模型，该 sample 的代码在 SDK 的 `msp/sample/npu/sample_yolov5s` 文件夹下：

```bash
root@ax650:~# /opt/bin/sample_npu_yolov5s -m /opt/data/npu/models/yolov5s.axmodel -i /opt/data/npu/images/dog.jpg -r 100
--------------------------------------
model file : /opt/data/npu/models/yolov5s.axmodel
image file : /opt/data/npu/images/dog.jpg
img_h, img_w : 640 640
--------------------------------------
Engine creating handle is done.
Engine creating context is done.
Engine get io info is done.
Engine alloc io is done.
Engine push input is done.
--------------------------------------
post process cost time:2.50 ms
--------------------------------------
Repeat 100 times, avg time 7.66 ms, max_time 7.69 ms, min_time 7.65 ms
--------------------------------------
detection num: 3
16:  91%, [ 138,  218,  310,  541], dog
 2:  69%, [ 470,   76,  690,  173], car
 1:  56%, [ 158,  120,  569,  420], bicycle
--------------------------------------
```

### 2.4 更多的模型示例

限于 SDK 的容量限制，DEMO 板只预置了此两项 DEMO；为了充分演示 AX650 系列芯片的潜力，更多类型的 DEMO 现已在 github 提供，详见 [Github AX Sample](https://github.com/AXERA-TECH/ax-samples.git)。
未完全统计，AX Sample 已经有如下多种示例：
- 物体检测
  - [YOLOv5s](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#yolov5s)
  - [YOLOv7-Tiny](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#YOLOv7-Tiny)
  - [YOLOv8s](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#YOLOv8s)
  - [YOLOX-S](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#YOLOX-S)
- 物体分割
  - [YOLOv5-Seg](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#YOLOv5-seg)
- 人脸检测
  - [SCRFD](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#Scrfd)
  - [YOLOv5-Face](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#YOLOv5-Face)
  - [YOLOv7-Face](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#YOLOv7-Face)
- 人体关键点
  - [HRNet](https://github.com/AXERA-TECH/ax-samples/blob/main/examples/ax650/README.md#HRNet)
