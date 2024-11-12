# ax650n_bsp_sdk

## What is this?

this is a ax650 linux bsp sdk form AX650_SDK_V1.45.0_P39. currently it is application layer open source.

## How to compile

### prepare arm gcc

```
wget https://developer.arm.com/-/media/Files/downloads/gnu-a/9.2-2019.12/binrel/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz
sudo tar -xvf gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu.tar.xz -C /opt/
export PATH="/opt/gcc-arm-9.2-2019.12-x86_64-aarch64-none-linux-gnu/bin/:$PATH"
````

### prepare compile files

```
git clone https://github.com/AXERA-TECH/ax650n_bsp_sdk.git
cd ax650n_bsp_sdk
```

Download the third-party zip file and decompress it into third-party dir

- [百度网盘](https://pan.baidu.com/s/1ikttjzp6aixdfed639_9dA?pwd=aq70)
- [Google Drive](https://drive.google.com/file/d/1cr9lvVKpfDc3JPyzEXuUQGyRUVqmXMBc/view?usp=sharing)

```
$ tree -L 2
.
├── LICENSE
├── README.md
├── app
│   ├── Makefile
│   └── demo
├── build
│   ├── Makefile
│   ├── axp_make.sh
│   ├── color.mk
│   ├── config.mak
│   ├── krules.mak
│   ├── make_ota_pkg.sh
│   ├── project.mak
│   ├── projects
│   ├── rules.mak
│   └── version.mak
├── msp
│   ├── component
│   ├── out
│   └── sample
└── third-party
    ├── Makefile
    ├── cmdline
    ├── opencv-4.5.5
    └── tinyalsa
```

### compile app and samples

compile app demo
```
cd app
make p=AX650_pipro_box
```

compile samples
```
cd samples
make p=AX650_pipro_box
```

the result 
```
ls msp/out/bin/
BoxDemo       sample_cipher  sample_dsp     sample_ives_s              sample_optee_hello_world  sample_pcie_icc_host   sample_pool_s               sample_uvc             sample_vin
FRTDemo       sample_cmm     sample_efuse   sample_ivps                sample_optee_sec_storage  sample_pcie_icc_slave  sample_pubkey_hash_write    sample_vdec            sample_vin_ivps_venc_rtsp
sample_adc    sample_cmm_s   sample_isp_3a  sample_npu_classification  sample_pcie_boot          sample_pcie_msg_host   sample_pubkey_hash_write_s  sample_vdec_ivps_venc  sample_vin_ivps_vo_venc
sample_audio  sample_dmadim  sample_ive     sample_npu_yolov5s         sample_pcie_dma_host      sample_pcie_msg_slave  sample_rtc                  sample_vdec_ivps_vo    sample_vin_vo
sample_avs    sample_dmaxor  sample_ives    sample_optee_aes           sample_pcie_dma_slave     sample_pool            sample_sysmap               sample_venc            sample_vo
```
