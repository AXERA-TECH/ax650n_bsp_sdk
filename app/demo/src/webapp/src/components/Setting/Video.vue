<template>
  <div>
    <div class="video-preview-container"></div>
    <div>
      <el-form ref="formVideoRef" :rules="formVideoRules" :model="formVideo" label-width="110px" size="mini">
        <el-form-item label="Camera:">
          <div id="cam_opt" >
            <el-select v-model="formVideo.src_id" @change="onChangeSrcID" :disabled="dual_mode == 0" size="mini">
              <div id="cam_opt_content" >
                <el-option v-for="item in formVideo.video_src_opts" :key="item.value" :label="item.label" :value="item.value"></el-option>
              </div>
            </el-select>
          </div>
        </el-form-item>
        <el-form-item label="启用主码流0" class="inline" v-show="formVideo.cap_list[0] > 0">
          <el-switch v-model="formVideo.video0.enable_stream"></el-switch>
        </el-form-item>
        <el-form-item label="" label-width="50px" v-show="formVideo.cap_list[0] > 0 && formVideo.video0.enable_stream" >
          <el-form-item label="编码类型:" class="inline">
            <div id="rc_encoder_type_0">
              <el-select v-model="formVideo.video0.encoder_type" style="width:91%" @change="onChangeEncRcType(0)" size="mini">
                <div id="rc_encoder_type_content_0">
                  <el-option v-for="item in formVideo.encode_opts" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
            <el-form-item label="码率(kbps):" class="inline">
            <el-input-number controls-position="right" :min="500" :max="15000" :step="500" v-model="formVideo.video0.bit_rate" style="width:91%" :disabled="!formVideo.video0.enable_stream" size="mini"></el-input-number>
          </el-form-item>
          <el-row>
            <el-form-item label="分辨率:" class="inline">
            <div id="resolution_opt_0" >
              <el-select v-model="formVideo.video0.resolution" style="width:91%" :disabled="!formVideo.video0.enable_stream || !formVideo.video0.enable_res_chg" size="mini">
                <div id="resolution_opt_content_0" >
                  <el-option v-for="item in formVideo.res_opt_0" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
            <el-form-item label="码流控制:" class="inline">
              <div id="rc_type_opt_0" >
                <el-select v-model="formVideo.video0.rc_type" style="width:91%" :disabled="false" @change="onChangeEncRcType(0)" size="mini">
                  <div id="rc_type_opt_content_0" >
                    <el-option v-for="item in formVideo.rc_type_options" :key="item.value" :label="item.label" :value="item.value"></el-option>
                  </div>
                </el-select>
              </div>
            </el-form-item>
          </el-row>
            <el-form-item label="min_qp:" class="inline" v-show="formVideo.video0.rc_type != 2">
              <div id="rc_min_qp_0" >
                <el-input-number controls-position="right" :min="0" :max=formVideo.video0.max_qp :step="10" v-model="formVideo.video0.min_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(0)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_qp:" class="inline" v-show="formVideo.video0.rc_type != 2">
              <div id="rc_max_qp_0" >
                <el-input-number controls-position="right" :min=formVideo.video0.min_qp :max="51" :step="10" v-model="formVideo.video0.max_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(0)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          <el-row>
            <el-form-item label="min_iqp:" class="inline" v-show="formVideo.video0.rc_type != 2 && formVideo.video0.encoder_type != 1">
              <div id="rc_min_iqp_0">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video0.max_iqp :step="10" v-model="formVideo.video0.min_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(0)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_iqp:" class="inline" v-show="formVideo.video0.rc_type != 2 && formVideo.video0.encoder_type != 1">
              <div id="rc_max_iqp_0">
                <el-input-number controls-position="right" :min=formVideo.video0.min_iqp :max="51" :step="10" v-model="formVideo.video0.max_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(0)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          </el-row>
            <el-form-item label="min_i_prop:" class="inline" v-show="formVideo.video0.rc_type === 0 && formVideo.video0.encoder_type != 1">
              <div id="rc_min_i_prop_0">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video0.max_iprop :step="10" v-model="formVideo.video0.min_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(0)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_i_prop:" class="inline" v-show="formVideo.video0.rc_type === 0 && formVideo.video0.encoder_type != 1">
              <div id="rc_max_i_prop_0">
                <el-input-number controls-position="right" :min=formVideo.video0.min_iprop :max="100" :step="10" v-model="formVideo.video0.max_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(0)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          </el-form-item>
        <el-row>
          <el-form-item label="启用子码流1" class="inline" v-show="formVideo.cap_list[1] > 0">
            <el-switch v-model="formVideo.video1.enable_stream"></el-switch>
          </el-form-item>
        </el-row>
        <el-form-item label="" label-width="50px" v-show="formVideo.cap_list[1] > 0 && formVideo.video1.enable_stream">
          <el-form-item label="编码类型:" class="inline">
            <div id="rc_encoder_type_1">
              <el-select v-model="formVideo.video1.encoder_type" style="width:91%" @change="onChangeEncRcType(0)" size="mini">
                <div id="rc_encoder_type_content_1">
                  <el-option v-for="item in formVideo.encode_opts" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
          <el-form-item label="码率(kbps):" class="inline">
            <el-input-number controls-position="right" :min="500" :max="15000" :step="500" v-model="formVideo.video1.bit_rate" style="width:91%" :disabled="!formVideo.video1.enable_stream" size="mini"></el-input-number>
          </el-form-item>
          <el-row>
          <el-form-item label="分辨率:" class="inline">
            <div id="resolution_opt_1" >
              <el-select v-model="formVideo.video1.resolution" style="width:91%" :disabled="!formVideo.video1.enable_stream || !formVideo.video1.enable_res_chg" size="mini">
                <div id="resolution_opt_content_1" >
                  <el-option v-for="item in formVideo.res_opt_1" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
            <el-form-item label="码流控制:" class="inline">
              <div id="rc_type_opt_1" >
                <el-select v-model="formVideo.video1.rc_type" style="width:91%" :disabled="false" @change="onChangeEncRcType(1)" size="mini">
                  <div id="rc_type_opt_content_1" >
                    <el-option v-for="item in formVideo.rc_type_options" :key="item.value" :label="item.label" :value="item.value"></el-option>
                  </div>
                </el-select>
              </div>
            </el-form-item>
          </el-row>
            <el-form-item label="min_qp:" class="inline" v-show="formVideo.video1.rc_type != 2">
            <div id="rc_min_qp_1">
              <el-input-number controls-position="right" :min="0" :max=formVideo.video1.max_qp :step="10" v-model="formVideo.video1.min_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(1)" size="mini"></el-input-number>
            </div>
          </el-form-item>
            <el-form-item label="max_qp:" class="inline" v-show="formVideo.video1.rc_type != 2">
            <div id="rc_max_qp_1">
              <el-input-number controls-position="right" :min=formVideo.video1.min_qp :max="51" :step="10" v-model="formVideo.video1.max_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(1)" size="mini"></el-input-number>
            </div>
          </el-form-item>
          <el-row>
            <el-form-item label="min_iqp:" class="inline" v-show="formVideo.video1.rc_type != 2 && formVideo.video1.encoder_type != 1">
            <div id="rc_min_iqp_1">
              <el-input-number controls-position="right" :min="0" :max=formVideo.video1.max_iqp :step="10" v-model="formVideo.video1.min_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(1)" size="mini"></el-input-number>
            </div>
          </el-form-item>
            <el-form-item label="max_iqp:" class="inline" v-show="formVideo.video1.rc_type != 2 && formVideo.video1.encoder_type != 1">
            <div id="rc_max_iqp_1">
              <el-input-number controls-position="right" :min=formVideo.video1.min_iqp :max="51" :step="10" v-model="formVideo.video1.max_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(1)" size="mini"></el-input-number>
            </div>
          </el-form-item>
          </el-row>
            <el-form-item label="min_i_prop:" class="inline" v-show="formVideo.video1.rc_type === 0 && formVideo.video1.encoder_type != 1">
              <div id="rc_min_i_prop_1">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video1.max_iprop :step="10" v-model="formVideo.video1.min_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(1)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_i_prop:" class="inline" v-show="formVideo.video1.rc_type === 0 && formVideo.video1.encoder_type != 1">
              <div id="rc_max_i_prop_1">
                <el-input-number controls-position="right" :min=formVideo.video1.min_iprop :max="100" :step="10" v-model="formVideo.video1.max_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(1)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          </el-form-item>
        <el-row>
          <el-form-item label="启用子码流2" class="inline" v-show="formVideo.cap_list[2] > 0">
            <el-switch v-model="formVideo.video2.enable_stream"></el-switch>
          </el-form-item>
        </el-row>
        <el-form-item label="" label-width="50px" v-show="formVideo.cap_list[2] > 0 && formVideo.video2.enable_stream">
          <el-form-item label="编码类型:" class="inline">
            <div id="rc_encoder_type_2">
              <el-select v-model="formVideo.video2.encoder_type" style="width:91%" @change="onChangeEncRcType(0)" size="mini">
                <div id="rc_encoder_type_content_2">
                  <el-option v-for="item in formVideo.encode_opts" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
          <el-form-item label="码率(kbps):" class="inline">
            <el-input-number controls-position="right" :min="500" :max="15000" :step="500" v-model="formVideo.video2.bit_rate" style="width:91%" :disabled="!formVideo.video2.enable_stream" size="mini"></el-input-number>
          </el-form-item>
          <el-row>
            <el-form-item label="分辨率:" class="inline">
            <div id="resolution_opt_2" >
              <el-select v-model="formVideo.video2.resolution" style="width:91%" :disabled="!formVideo.video2.enable_stream || !formVideo.video2.enable_res_chg" size="mini">
                <div id="resolution_opt_content_2" >
                  <el-option v-for="item in formVideo.res_opt_2" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
            <el-form-item label="码流控制:" class="inline">
              <div id="rc_type_opt_2" >
                <el-select v-model="formVideo.video2.rc_type" style="width:91%" :disabled="false" @change="onChangeEncRcType(2)" size="mini">
                  <div id="rc_type_opt_content_2" >
                    <el-option v-for="item in formVideo.rc_type_options" :key="item.value" :label="item.label" :value="item.value"></el-option>
                  </div>
                </el-select>
              </div>
            </el-form-item>
          </el-row>
            <el-form-item label="min_qp:" class="inline" v-show="formVideo.video2.rc_type != 2">
              <div id="rc_min_qp_2">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video2.max_qp :step="10" v-model="formVideo.video2.min_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(2)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_qp:" class="inline" v-show="formVideo.video2.rc_type != 2">
              <div id="rc_max_qp_2">
                <el-input-number controls-position="right" :min=formVideo.video2.min_qp :max="51" :step="10" v-model="formVideo.video2.max_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(2)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          <el-row>
            <el-form-item label="min_iqp:" class="inline" v-show="formVideo.video2.rc_type != 2 && formVideo.video2.encoder_type != 1">
              <div id="rc_min_iqp_2">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video2.max_iqp :step="10" v-model="formVideo.video2.min_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(2)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_iqp:" class="inline" v-show="formVideo.video2.rc_type != 2 && formVideo.video2.encoder_type != 1">
              <div id="rc_max_iqp_2">
                <el-input-number controls-position="right" :min=formVideo.video2.min_iqp :max="51" :step="10" v-model="formVideo.video2.max_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(2)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          </el-row>
            <el-form-item label="min_i_prop:" class="inline" v-show="formVideo.video2.rc_type === 0 && formVideo.video2.encoder_type != 1">
              <div id="rc_min_i_prop_2">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video2.max_iprop :step="10" v-model="formVideo.video2.min_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(2)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_i_prop:" class="inline" v-show="formVideo.video2.rc_type === 0 && formVideo.video2.encoder_type != 1">
              <div id="rc_max_i_prop_2">
                <el-input-number controls-position="right" :min=formVideo.video2.min_iprop :max="100" :step="10" v-model="formVideo.video2.max_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(2)" size="mini"></el-input-number>
              </div>
            </el-form-item>
        </el-form-item>
        <el-row>
          <el-form-item label="启用子码流3" class="inline" v-show="formVideo.cap_list[3] > 0">
            <el-switch v-model="formVideo.video3.enable_stream"></el-switch>
          </el-form-item>
        </el-row>
        <el-form-item label="" label-width="50px" v-show="formVideo.cap_list[3] > 0 && formVideo.video3.enable_stream">
          <el-form-item label="编码类型:" class="inline">
            <div id="rc_encoder_type_3">
              <el-select v-model="formVideo.video3.encoder_type" style="width:91%" size="mini">
                <div id="rc_encoder_type_content_3">
                  <el-option v-for="item in formVideo.encode_opts" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
          <el-form-item label="码率(kbps):" class="inline">
            <el-input-number controls-position="right" :min="500" :max="15000" :step="500" v-model="formVideo.video3.bit_rate" style="width:91%" :disabled="!formVideo.video3.enable_stream" size="mini"></el-input-number>
          </el-form-item>
          <el-row>
            <el-form-item label="分辨率:" class="inline">
            <div id="resolution_opt_3" >
              <el-select v-model="formVideo.video3.resolution" style="width:91%" :disabled="!formVideo.video3.enable_stream || !formVideo.video3.enable_res_chg" size="mini">
                <div id="resolution_opt_content_3" >
                  <el-option v-for="item in formVideo.res_opt_3" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                </div>
              </el-select>
            </div>
          </el-form-item>
            <el-form-item label="码流控制:" class="inline">
              <div id="rc_type_opt_3" >
                <el-select v-model="formVideo.video3.rc_type" style="width:91%" :disabled="false" size="mini">
                  <div id="rc_type_opt_content_3" >
                    <el-option v-for="item in formVideo.rc_type_options" :key="item.value" :label="item.label" :value="item.value"></el-option>
                  </div>
                </el-select>
              </div>
            </el-form-item>
            <el-form-item label="min_qp:" class="inline" v-show="formVideo.video3.rc_type === 0 && formVideo.video3.encoder_type != 1">
              <div id="rc_min_qp_3">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video3.max_qp :step="10" v-model="formVideo.video3.min_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(3)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_qp:" class="inline" v-show="formVideo.video3.rc_type === 0 && formVideo.video3.encoder_type != 1">
              <div id="rc_max_qp_3">
                <el-input-number controls-position="right" :min=formVideo.video3.min_qp :max="100" :step="10" v-model="formVideo.video3.max_qp" style="width:91%" :disabled="false" @change="onChangeRcVal(3)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          <el-row>
            <el-form-item label="min_iqp:" class="inline" v-show="formVideo.video3.rc_type === 0 && formVideo.video3.encoder_type != 1">
              <div id="rc_min_iqp_3">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video3.max_iqp :step="10" v-model="formVideo.video3.min_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(3)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_iqp:" class="inline" v-show="formVideo.video3.rc_type === 0 && formVideo.video3.encoder_type != 1">
              <div id="rc_max_iqp_3">
                <el-input-number controls-position="right" :min=formVideo.video3.min_iqp :max="100" :step="10" v-model="formVideo.video3.max_iqp" style="width:91%" :disabled="false" @change="onChangeRcVal(3)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          </el-row>
            <el-form-item label="min_i_prop:" class="inline" v-show="formVideo.video3.rc_type === 0 && formVideo.video3.encoder_type != 1">
              <div id="rc_min_i_prop_3">
                <el-input-number controls-position="right" :min="0" :max=formVideo.video3.max_iprop :step="10" v-model="formVideo.video3.min_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(3)" size="mini"></el-input-number>
              </div>
            </el-form-item>
            <el-form-item label="max_i_prop:" class="inline" v-show="formVideo.video3.rc_type === 0 && formVideo.video3.encoder_type != 1">
              <div id="rc_max_i_prop_3">
                <el-input-number controls-position="right" :min=formVideo.video3.min_iprop :max="100" :step="10" v-model="formVideo.video3.max_iprop" style="width:91%" :disabled="false" @change="onChangeRcVal(3)" size="mini"></el-input-number>
              </div>
            </el-form-item>
          </el-row>
        </el-form-item>
        <el-form-item>
          <el-divider></el-divider>
          <el-button type="primary" @click="onSubmit">修改</el-button>
        </el-form-item>
      </el-form>
    </div>
  </div>
</template>

<script>
export default {
  props: ["dual_mode"],
  data() {
    return {
      formVideo: {
        video_src_opts: [
          {
            label: '0',
            value: 0
          },
          {
            label: '1',
            value: 1
          }
        ],
        rc_type_options: [
          {
            label: 'CBR',
            value: 0
          },
          {
            label: 'VBR',
            value: 1
          },
          {
            label: 'FIXQP',
            value: 2
          }
        ],
        encode_opts: [
          {
            label: 'H264',
            value: 0
          },
          {
            label: 'MJPEG',
            value: 1
          },
          {
            label: 'H265',
            value: 2
          }
        ],
        src_id: 0,
        res_opt_0: [],
        res_opt_1: [],
        res_opt_2: [],
        res_opt_3: [],
        cap_list: [0, 0, 0, 0],
        video0: {
          enable_stream: false,
          enable_res_chg: false,
          encoder_type: 0,
          bit_rate: 0,
          resolution: '',
          resolution_opt: [],
          enc_rc_info: [],
          rc_type: 0,
          min_qp: 0,
          max_qp: 51,
          min_iqp: 0,
          max_iqp: 51,
          min_iprop: 10,
          max_iprop: 40
        },
        video1: {
          enable_stream: false,
          enable_res_chg: false,
          encoder_type: 0,
          bit_rate: 0,
          resolution: '',
          resolution_opt: [],
          enc_rc_info: [],
          rc_type: 0,
          min_qp: 0,
          max_qp: 51,
          min_iqp: 0,
          max_iqp: 51,
          min_iprop: 10,
          max_iprop: 40
        },
        video2: {
          enable_stream: false,
          enable_res_chg: false,
          encoder_type: 0,
          bit_rate: 0,
          resolution: '',
          resolution_opt: [],
          enc_rc_info: [],
          rc_type: 0,
          min_qp: 0,
          max_qp: 51,
          min_iqp: 0,
          max_iqp: 51,
          min_iprop: 10,
          max_iprop: 40
        },
        video3: {
          enable_stream: false,
          enable_res_chg: false,
          encoder_type: 0,
          bit_rate: 0,
          resolution: '',
          resolution_opt: [],
          enc_rc_info: [],
          rc_type: 0,
          min_qp: 0,
          max_qp: 51,
          min_iqp: 0,
          max_iqp: 51,
          min_iprop: 10,
          max_iprop: 40
        }
      },
      formVideoRules: {
      }
    }
  },
  created() {
    console.log('video++')
    this.getInfo()
    console.log('video--')
  },
  methods: {
    async onSubmit() {
      try {
        var objData = {}
        objData = JSON.parse(JSON.stringify(this.formVideo))
        const { data: res } = await this.$http.post('setting/video', objData)
        console.log('video get return: ', res)
        if (res.meta.status === 200) {
          this.$message.success('修改成功')
        } else {
          this.$message.success('修改失败')
        }
      } catch (error) {
        this.$message.error('修改失败')
      }
    },
    async getInfo() {
      try {
        const { data: res } = await this.$http.get('setting/video', { params: { src_id: this.formVideo.src_id } })
        console.log('video get return: ', res)
        if (res.meta.status === 200) {
          this.formVideo.cap_list = res.data.cap_list

          if (this.formVideo.cap_list[0] > 0) {
            this.formVideo.video0 = res.data.video0
          }

          if (this.formVideo.cap_list[1] > 0) {
            this.formVideo.video1 = res.data.video1
          }

          if (this.formVideo.cap_list[2] > 0) {
            this.formVideo.video2 = res.data.video2
          }

          if (this.formVideo.cap_list[3] > 0) {
            this.formVideo.video3 = res.data.video3
          }

          this.init_resolution_options()
          this.init_rc_infos()

        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    },
    init_rc_infos() {
      var _video_info_array = [this.formVideo.video0
        , this.formVideo.video1
        , this.formVideo.video2]
      for (let i = 0; i < _video_info_array.length; i++) {
        for (let j = 0; j < _video_info_array[i].enc_rc_info.length; j++) {
          let rc_info = _video_info_array[i].enc_rc_info[j]

          if (_video_info_array[i].encoder_type != rc_info.encoder_type) {
            continue
          }

          for (let k = 0; k < rc_info.rc.length; k++) {
            if (rc_info.rc[k].rc_type != _video_info_array[i].rc_type) {
              continue
            }
            _video_info_array[i].min_qp = rc_info.rc[k].min_qp
            _video_info_array[i].max_qp = rc_info.rc[k].max_qp
            _video_info_array[i].min_iqp = rc_info.rc[k].min_iqp
            _video_info_array[i].max_iqp = rc_info.rc[k].max_iqp
            _video_info_array[i].min_iprop = rc_info.rc[k].min_iprop
            _video_info_array[i].max_iprop = rc_info.rc[k].max_iprop
          }
        }
        console.log("_video_info_array: " + _video_info_array[i])
      }
    },
    init_resolution_options() {
      var _video_info_array = [this.formVideo.video0
        , this.formVideo.video1
        , this.formVideo.video2
        , this.formVideo.video3]

      for (var i = 0; i < _video_info_array.length; i++) {
        switch (i) {
          case 0:
            this.formVideo.res_opt_0 = []
            break;
          case 1:
            this.formVideo.res_opt_1 = []
            break;
          case 2:
            this.formVideo.res_opt_2 = []
            break;
          case 3:
            this.formVideo.res_opt_3 = []
            break;
          default:
            break;
        }

        for (var j = 0; j < _video_info_array[i].resolution_opt.length; j++) {
          var _res = _video_info_array[i].resolution_opt[j].trim() + ''
          var _item = { label: _res, value: _res }
          switch (i) {
            case 0:
              this.formVideo.res_opt_0.push(_item)
              break;
            case 1:
              this.formVideo.res_opt_1.push(_item)
              break;
            case 2:
              this.formVideo.res_opt_2.push(_item)
              break;
            case 3:
              this.formVideo.res_opt_3.push(_item)
              break;
            default:
              break;
          }
        }
      }
    },
    onChangeSrcID() {
      this.getInfo()
    },
    onChangeEncRcType(channel) {
      let _video_info_array = [this.formVideo.video0
        , this.formVideo.video1
        , this.formVideo.video2]

      {
        for (let j = 0; j < _video_info_array[channel].enc_rc_info.length; j++) {
          let rc_info = _video_info_array[channel].enc_rc_info[j]
          if (_video_info_array[channel].encoder_type != rc_info.encoder_type) {
            continue
          }

          for (let k = 0; k < rc_info.rc.length; k++) {
            if (rc_info.rc[k].rc_type != _video_info_array[channel].rc_type) {
              continue
            }
            let _rc = rc_info.rc[k]
            _video_info_array[channel].min_qp = _rc.min_qp;
            _video_info_array[channel].max_qp = _rc.max_qp;

            _video_info_array[channel].min_iqp = _rc.min_iqp;
            _video_info_array[channel].max_iqp = _rc.max_iqp;

            _video_info_array[channel].min_iprop = _rc.min_iprop;
            _video_info_array[channel].max_iprop = _rc.max_iprop;
          }
        }
      }
    },
    onChangeRcVal(channel) {
      let _video_info_array = [this.formVideo.video0
        , this.formVideo.video1
        , this.formVideo.video2]
      console.log("c_type: " + _video_info_array[channel].rc_type + ",min_qp " + _video_info_array[channel].min_qp + ", max_qp: " + _video_info_array[channel].max_qp + ",min_iqp " + _video_info_array[channel].min_iqp + ", max_iqp: " + _video_info_array[channel].max_iqp + ",min_iprop " + _video_info_array[channel].min_iprop + ", max_iprop: " + _video_info_array[channel].max_iprop)
      for (let j = 0; j < _video_info_array[channel].enc_rc_info.length; j++) {
        let rc_info = _video_info_array[channel].enc_rc_info[j]
        if (_video_info_array[channel].encoder_type != rc_info.encoder_type) {
          continue
        }

        for (let k = 0; k < rc_info.rc.length; k++) {
          if (rc_info.rc[k].rc_type != _video_info_array[channel].rc_type) {
            continue
          }
          let _rc = rc_info.rc[k]

          _rc.min_qp = _video_info_array[channel].min_qp;
          _rc.max_qp = _video_info_array[channel].max_qp;

          _rc.min_iqp = _video_info_array[channel].min_iqp;
          _rc.max_iqp = _video_info_array[channel].max_iqp;

          _rc.min_iprop = _video_info_array[channel].min_iprop;
          _rc.max_iprop = _video_info_array[channel].max_iprop;

        }
      }
    }
  }
}
</script>

<style lang="less" scoped>
.el-input-number {
  width: 100px;
}

.el-select {
  width: 110px;
}

.inline {
  display: inline-block;
  width: 30%
}
.el-form-item{
  margin-bottom: 5px;
}
</style>
