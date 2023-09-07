## 功能说明：
- isp_3a文件夹下面的代码, 是爱芯SDK包提供的示例参考代码, 方便客户快速的理解3A整个模块的配置流程.
示例代码演示的是如下的pipeline形态：单摄、双摄双4K.
目前仅提供demo板适配的sensor的初始化配置: OV-OS08A20，如有需要可以按照下文<font color=#0000CD>“加入新sensor步骤”</font>
- AE,LSC,AWB需要根据AX_ISP_AE_REGFUNCS_T，AX_ISP_LSC_REGFUNCS_T,AX_ISP_AWB_REGFUNCS_T里面定义的函数指针，编写init,run,deinit函数，然后参照sample_isp_3a.c 将AE，LSC，AWB注册到ISP的pipline当中。AF只需跑在单独的线程中，通过AX_ISP_IQ_GetAF1Statistics获取AF统计值，当有AF统计值时，会立即返回；否则会阻塞200ms<font color=#0000CD>具体如何使用请参见《23 - AX 3A 开发指南.docx》</font>



## 使用示例：
单摄单路OS08A20 8M的sensor, 跑一路SDR, 运行命令如下：
./sample_isp_3a -c 1 -m 1 -e 1
- -c： 选择不同的场景:
    1-单摄OS08A20
    2-双摄OS08A20
- -m:  选择不同的模式:
    0-LoadRaw 模式
    1-sensor 模式
    2-tpg模式
- -u:  选择是否使用客制化3A:
    0-不使用客制化3A
    1-使用客制化3A(未配置-u时默认开启)
- -e:  选择SDR/HDR模式:
    1-SDR (默认运行SDR)
    2-HDR 2DOL

## 加入新sensor步骤：
<font color=#0000CD>加入新sensor支持步骤,详细的代码修改参见文档《20 - AX Sensor 调试指南.docx》</font>

- 配置Sensor对应的common pool
- 配置Sensor、VIN、Chn的参数
- 注册Sensor库
- common子函数中添加对应的Sensor Case