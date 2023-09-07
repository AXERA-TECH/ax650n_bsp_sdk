#BoxDemo
## 如何执行？
1. cd /opt/bin/BoxDemo
2. vi box.conf 配置日志级别以及检测开关等。[参数说明](#配置参数)
3. 执行./run.sh

## 如何编译？
1. cd app/demo/src/ppl/box
2. make p=xxx clean
3. make p=xxx
4. make p=xxx install
> p=xxx 指定编译项目名，示例：make p=AX650_emmc

# <a href="#配置参数">配置参数</a>

|   #   |             参数       |   参数范围   |                          说明           |
| ----- | ----------------------| ------------ | --------------------------------------- |
|   1   | [DETECT]enable        |              | 0：不检测 1:检测                         |
|   2   | [DISPC]dev            | [0 - 1]      | 0: HDMI-0 1:HDMI-1                      |
|   3   | [STREAM]count         | [1 - 25]     | 码流数量，目前建议不超过12                |