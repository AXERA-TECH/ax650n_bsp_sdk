#PCIe sample 主要用来测试pcie host 与slave 间消息传输，数据传输，pcie boot。
#pcie sample 分以下几部分：
    1. dma 模块: 测试host 与slave 间dma 数据传输
         ep: 存放salve dma sample
         rc: 存放host dma sample
         rc_x86：当ax650做ep 时，放在第三方平台的host dma sample
    2. msg 模块：测试host 与slave 间基于共享内存的消息传输
         ep: 存放slave msg sample
         rc: 存放host msg sample
         rc_x86: 当ax650做ep 时，放在第三方平台的host msg sample
    3. boot 模块:
         用于host pcie 启动 slave， 当host 是第三方平台时需将boot sample 放到第三方平台编译

##msg 运行方法:
    先加载host，slave 端驱动模块,再加载sample
    驱动模块位置：/soc/ko
    slave:
        insmod ax_pcie_slave_dev.ko                                      //加载slave device driver
        mount -t configfs none /sys/kernel/config/                       //mount configfs
        cd /sys/kernel/config/pci_ep/
        mkdir functions/ax_pcie_ep/func1                                 //创建一个function, 也就是一个device
        echo 0x1f4b >functions/ax_pcie_ep/func1/vendorid                 //修改device vendorid
        echo 0x0650 >functions/ax_pcie_ep/func1/deviceid                 //修改device deviceid
        echo 16 >functions/ax_pcie_ep/func1/msi_interrupts               //设置msi 中断数量，一共32个
        ln -s functions/ax_pcie_ep/func1/ controllers/40000000.pcie_ep/  //function 与controller 建立链接
        echo 1 >controllers/40000000.pcie_ep/start                       //设置完成后，开启LTSSM, start link traning
        insmod ax_pcie_common.ko                                         //加载common init driver
        insmod ax_pcie_msg.ko                                            //加载消息driver
    host:
        echo 1 >/sys/bus/pci/rescan                              //重新扫描slave device, 需在slave加载完后扫描
        insmod ax_pcie_host_dev.ko                               //加载device 设备驱动
        insmod ax_pcie_common.ko
        insmod ax_pcie_msg.ko
        insmod ax_pcie_mmb.ko
    sample:
        先执行slave端msg sample: sample_pcie_msg_slave
        再执行host端msg sample: sample_pcie_msg_host
        note: 需要执行压力测试和多线程case时，需要传一个max_loop值 sample_pcie_msg_host [loop]
        执行后会有以下case 选择：
        0: Test write: host -> slave.
        1: Test read : host <- slave.
        2: Test copy : host -> slave -> host.
        3: pressure test: test ring buffer.        //ringbuffer 压力测试
        4: Multi-thread test.
        please enter one key to test:
    success log:
        Sample test failed!
    failed log:
        Sample test success!
    script:
        在/opt/script目录下提供pcie_load_drv.sh执行以上驱动加载，在脚本执行完后可执行对应sample进行case测试。
        ./pcie_load_drv.sh -i    insmod driver
        ./pcie_load_drv.sh -r    rmmod driver


##dma 运行方法：
    先加载host，slave 端驱动模块,再加载sample
    驱动模块位置：/soc/ko
    slave:
        insmod ax_pcie_slave_dev.ko                                      //加载slave device driver
        mount -t configfs none /sys/kernel/config/                       //mount configfs
        cd /sys/kernel/config/pci_ep/
        mkdir functions/ax_pcie_ep/func1                                 //创建一个function, 也就是一个device
        echo 0x1f4b >functions/ax_pcie_ep/func1/vendorid                 //修改device vendorid
        echo 0x0650 >functions/ax_pcie_ep/func1/deviceid                 //修改device deviceid
        echo 16 >functions/ax_pcie_ep/func1/msi_interrupts               //设置msi 中断数量，一共32个
        ln -s functions/ax_pcie_ep/func1/ controllers/40000000.pcie_ep/  //function 与controller 链接
        echo 1 >controllers/40000000.pcie_ep/start                       //设置完成后，开启LTSSM, start link traning
        insmod ax_pcie_common.ko                                         //加载common init driver
        insmod ax_pcie_msg.ko                                            //加载消息driver
        insmod ax_pcie_dma_slave.ko                                      //加载dma driver
        insmod ax_pcie_mmb.ko                                            //加载memory 申请driver, 此driver仅用与测试
    host:
        echo 1 >/sys/bus/pci/rescan                              //重新扫描slave device
        insmod ax_pcie_host_dev.ko                               //加载device 设备驱动
        insmod ax_pcie_common.ko
        insmod ax_pcie_msg.ko
        insmod ax_pcie_mmb.ko
    sample:
        先执行slave端dma sample: sample_pcie_dma_slave
        再执行host端dma sample: sample_pcie_dma_host
        执行后会有以下case 选择:
        please enter one key to test:
        s: send file test.                     //host 发送一个文件,需要提前准备send-file文件
        r: recv file test.                     //host 接收一个文件
        l: loopback data test.                 //loopback 测试，先写后读
        d: dma link list send test.            //link list send 测试
        e: dma link list recv test.            //link list recv 测试
        m: dma multi-task send test.           //多dma task send测试
        w: dma multi-task recv test.           //多dma task send测试
        x: dma multi-task loopback test.       //多dma task loopback测试
        p: pressure test.                      //压力测试
        b: multi-task pressure test.
        t: multi-thread test.
        q: quit test, ep app will also exit.   //退出
    success log:
        pcie dma write success!
        pcie dma read success!
        send write_done msg success
        send read_done msg success
    failed log:
        pcie dma write failed!
        pcie dma read failed!
        send write_done msg failed!
        send read_done msg failed!
    script:
        在/opt/script目录下提供pcie_load_drv.sh执行以上驱动加载，在脚本执行完后可执行对应sample进行case测试。
        ./pcie_load_drv.sh -i    insmod driver
        ./pcie_load_drv.sh -r    rmmod driver

#boot 运行方法：
    先加载host，boot驱动模块，再加载sample:
    host:
        insmod ax_pcie_host_dev.ko                               //加载device 设备驱动
        insmod ax_pcie_boot.ko                                  //加载boot驱动
    sample：
        ./sample_pcie_boot <image bin> <dest addr>
        ...
        ./sample_pcie_boot start   //开始启动，不执行这条命令不会启动

    success log：
        tranfer xxx to devices success.   //会打印传输哪个image出错
    failed log:
        tranfer xxx to devices failed.
        err type:
            HDMA read original header fail
            HDMA read backup header fail
            Header checksum fail
            HDMA read ce fw fail
            CE fw checksum fail
            Eip init fail
            Public key verify fail
            HDMA read image fail
            Image checksum fail
            CE hash verification fail
            Image decrypto fail
            Other err type
    script:
    在/opt/script目录下提供pcie_load_drv.sh执行以上驱动加载，在脚本执行完后可执行对应sample进行case测试。
    ./pcie_load_drv.sh -i    insmod driver
    ./pcie_load_drv.sh -r    rmmod driver
