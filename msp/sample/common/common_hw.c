#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ax_isp_api.h"
#include "common_vin.h"

AX_S32 COMMON_VIN_GetApdPlateId(AX_CHAR *apd_plate_id)
{
    FILE *pFile = NULL;
    AX_CHAR temp[BOARD_ID_LEN] = {0};

    pFile = fopen("/proc/ax_proc/apd_plate_id", "r");
    if (pFile) {
        fread(temp, BOARD_ID_LEN, 1, pFile);
        fclose(pFile);
    } else {
        COMM_VIN_PRT("fopen /proc/ax_proc/apd_plate_id failed!!!\n");
        return -1;
    }

    strncpy(apd_plate_id, temp, BOARD_ID_LEN * sizeof(AX_CHAR));
    return 0;
}

AX_S32 COMMON_VIN_GetSensorResetGpioNum(AX_U8 nDevId)
{
    AX_CHAR apd_plate_id[BOARD_ID_LEN] = {0};
    COMMON_VIN_GetApdPlateId(apd_plate_id);

    if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE", sizeof("ADP_RX_DPHY_2X4LANE") - 1)
        || !strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE_N", sizeof("ADP_RX_DPHY_2X4LANE_N") - 1)) {
        if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X4LANE_N", sizeof("ADP_RX_DPHY_2X4LANE_N") - 1)) {
            switch (nDevId) {
            case 0:
                return 64;
            case 2:
                return 65;
            default:
                return 64;
            }
        } else {
            switch (nDevId) {
            case 0:
                return 64;
            case 4:
                return 65;
            default:
                return 64;
            }
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DPHY_8X2LANE", sizeof("ADP_RX_DPHY_8X2LANE") - 1)
               || !strncmp(apd_plate_id, "ADP_RX_CPHY_8X2TRIO", sizeof("ADP_RX_CPHY_8X2TRIO") - 1)) {
        switch (nDevId) {
        case 0:
            return 64;
        case 1:
            return 65;
        case 2:
            return 22;
        case 3:
            return 135;
        case 4:
            return 130;
        case 5:
            return 131;
        case 6:
            return 132;
        case 7:
            return 133;
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DPHY_4X4LANE", sizeof("ADP_RX_DPHY_4X4LANE") - 1)
               || !strncmp(apd_plate_id, "ADP_RX_CPHY_4X3TRIO", sizeof("ADP_RX_CPHY_4X3TRIO") - 1)) {
        switch (nDevId) {
        case 0:
            return 64;
        case 2:
            return 65;
        case 4:
            return 22;
        case 6:
            return 135;
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DPHY_2X8_LVDS_1X16LANE", sizeof("ADP_RX_DPHY_2X8_LVDS_1X16LANE") - 1)) {
        switch (nDevId) {
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DVP_1X10BIT_DPHY_1X2LANE_BT601_656_1120", sizeof("ADP_RX_DVP_1X10BIT_DPHY_1X2LANE_BT601_656_1120") - 1)) {
        switch (nDevId) {
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT1120_2X10BIT", sizeof("ADP_RX_BT1120_2X10BIT") - 1)) {
        switch (nDevId) {
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT1120_2X8BIT_DPHY_2X2LANE", sizeof("ADP_RX_BT1120_2X8BIT_DPHY_2X2LANE") - 1)) {
        switch (nDevId) {
        case 3:
            return 135;
        case 7:
            return 133;
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT601_656_2X10BIT_DPHY_2X2LANE", sizeof("ADP_RX_BT601_656_2X10BIT_DPHY_2X2LANE") - 1)) {
        switch (nDevId) {
        case 2:
            return 22;
        case 6:
            return 132;
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_BT601_656_2X8BIT_DPHY_4X2LANE", sizeof("ADP_RX_BT601_656_2X8BIT_DPHY_4X2LANE") - 1)) {
        switch (nDevId) {
        case 2:
            return 22;
        case 3:
            return 135;
        case 6:
            return 132;
        case 7:
            return 133;
        default:
            return 64;
        }
    } else if (!strncmp(apd_plate_id, "ADP_RX_DVP_16bit_HDMI_TO_DPHY_2X4LANE", sizeof("ADP_RX_DVP_16bit_HDMI_TO_DPHY_2X4LANE") - 1)) {
        switch (nDevId) {
        default:
            return 64;
        }
    }

    return 64;
}

