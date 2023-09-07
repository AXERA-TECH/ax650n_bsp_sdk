1. Function description:

    This module is an example code for VIN->IVPS->VO->VENC, which is convenient for users to quickly understand
and master the use of VIN->IVPS->VO->VENC.


1. Examples:

Example1: VIN->IVPS->VO->VENC  [Use default display interface setting which is hdmi 1080p of vo dpu0.]
	/opt/bin/sample_vin_ivps_vo_venc -c 0
Example2: VIN->IVPS->VO->VENC  [Use display interface setting which is hdmi 1080p of vo dpu1.]
    /opt/bin/sample_vin_ivps_vo_venc -c 0 -v hdmi@1080P60@dev1


1. Results:

    After running successfully, vo will display stream on the screen. The encoded code stream will be saved in the
work directory. The saved stream file is named with vo_venc_chn.

	Execute Ctrl + C to exit.


1. Notes:


