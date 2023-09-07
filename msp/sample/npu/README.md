# AX-Samples-AX650

`AX-Samples-AX650` 提供运行 `Pulsar2` 编译生成的 `axmodel` 模型的 example 代码，是 `AX650` 智能算法模型运行的必要示例。

已验证硬件平台

- AX650

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

登录 AX650A 开发板为例，进入 `/opt/bin` 文件夹。

- 将 **Pulsar2** 生成的 **axmodel** 模型拷贝到  `/opt/bin` 路径下；
- 将测试图片拷贝到 `/opt/bin` 路径下。

```
/opt/bin # ls -l
total 26628
-rwxrw-r--    1 1000     1000       5713512 Nov  4  2022 sample_npu_classification
-rwxrw-r--    1 1000     1000       5921656 Nov  4  2022 sample_npu_yolov5s
-rw-rw-r--    1 1000     1000        140391 Nov  4  2022 cat.jpg
-rw-------    1 1000     root        163759 Oct 17  2022 dog.jpg
-rw-rw-r--    1 1000     1000       5355828 Nov  4  2022 mobilenetv2.axmodel
-rw-------    1 1000     root       9772840 Oct 17  2022 yolov5s.axmodel
```

### 2.2 分类模型

```bash
/opt/bin # ./sample_npu_classification -m mobilenetv2.axmodel -i cat.jpg --repeat 100
--------------------------------------
model file : mobilenetv2.axmodel
image file : cat.jpg
img_h, img_w : 224 224
--------------------------------------
Engine creating handle is done.
Engine creating context is done.
Engine get io info is done.
Engine alloc io is done.
Engine push input is done.
--------------------------------------
topk cost time:0.10 ms
9.7150, 283
9.3965, 285
8.9187, 281
8.2816, 282
7.4853, 463
--------------------------------------
Repeat 100 times, avg time 0.82 ms, max_time 0.83 ms, min_time 0.82 ms
--------------------------------------
```

### 2.3 检测模型

```bash
/opt/bin # ./sample_npu_yolov5s -m yolov5s.axmodel -i dog.jpg -r 100
--------------------------------------
model file : yolov5s.axmodel
image file : dog.jpg
img_h, img_w : 640 640
--------------------------------------
Engine creating handle is done.
Engine creating context is done.
Engine get io info is done.
Engine alloc io is done.
Engine push input is done.
--------------------------------------
post process cost time:2.41 ms
--------------------------------------
Repeat 100 times, avg time 8.16 ms, max_time 8.17 ms, min_time 8.15 ms
--------------------------------------
detection num: 3
16:  92%, [ 133,  211,  313,  546], dog
 2:  72%, [ 468,   73,  691,  174], car
 1:  56%, [ 163,  123,  571,  415], bicycle
--------------------------------------
```
