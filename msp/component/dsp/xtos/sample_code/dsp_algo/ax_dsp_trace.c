/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/


#include "ax_dsp_trace.h"

static int current_log_level = 0; // AX_DBG_INFO

void AX_SET_LOG_LEVEL(int level) { current_log_level = level; }

int AX_GET_LOG_LEVEL() { return current_log_level; }

int AX_IN_LOG_LEVEL(int level) {
  if (level >= current_log_level) {
    return 1;
  }

  return 0;
}

const char *AX_GET_LEVEL_STR(int level) {
  switch (level) {
  case 7: {
    return "EMERG";
    break;
  }
  case 6: {
    return "ALERT";
    break;
  }
  case 5: {
    return "CRIT";
    break;
  }
  case 4: {
    return "ERR";
    break;
  }
  case 3: {
    return "WARN";
    break;
  }
  case 2: {
    return "NOTICE";
    break;
  }
  case 1: {
    return "INFO";
    break;
  }
  case 0: {
    return "DEBUG";
    break;
  }
  default: {
    return "UnKnown";
    break;
  }
  }

  return "UnKnown";
}

const char *GetComputeErrorInfo(int status) {
  switch (status) {
  case 0: {
    return ("no error");
    break;
  }
  case 1: {
    return ("input alignment requirements are not satisfied");
    break;
  }
  case 2: {
    return ("output alignment requirements are not satisfied");
    break;
  }
  case 3: {
    return ("same modulo alignment requirement is not satisfied");
    break;
  }
  case 4: {
    return ("arguments are somehow invalid");
    break;
  }
  case 5: {
    return ("tile is not placed in local memory");
    break;
  }
  case 6: {
    return ("inplace operation is not supported");
    break;
  }
  case 7: {
    return ("edge extension size is too small");
    break;
  }
  case 8: {
    return ("input/output tile size is too small or too big or otherwise "
            "inconsistent");
    break;
  }
  case 9: {
    return ("temporary tile size is too small or otherwise inconsistent");
    break;
  }
  case 10: {
    return ("filer kernel size is not supported");
    break;
  }
  case 11: {
    return ("invalid normalization divisor or shift value");
    break;
  }
  case 12: {
    return ("invalid coordinates");
    break;
  }
  case 13: {
    return ("the transform is singular or otherwise invalid");
    break;
  }
  case 14: {
    return ("one of required arguments is null");
    break;
  }
  case 15: {
    return ("threshold value is somehow invalid");
    break;
  }
  case 16: {
    return ("provided scale factor is not supported");
    break;
  }
  case 17: {
    return ("tile size can lead to sum overflow");
    break;
  }
  case 18: {
    return ("the requested functionality is absent in current version");
    break;
  }
  case 19: {
    return ("invalid channel number");
    break;
  }
  case 20: {
    return ("argument has invalid data type");
    break;
  }
  case 21: {
    return ("No suitable variant found for the function");
    break;
  }
  default: {
    return ("Incorrect error flag\n");
    break;
  }
  }
}
