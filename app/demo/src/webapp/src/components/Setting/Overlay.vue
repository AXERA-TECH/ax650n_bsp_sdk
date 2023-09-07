<template>
  <div>
    <el-container>
      <el-container>
        <el-aside class="preview_container" width="600px">
          <div class="videoContainer" id="videoContainer">
            <div id="osdVideo" class="osdVideo" ref="osdVideoRef">
              <video id="myVideo" v-show="this.showVideo"  ref="axVideoRef" class="axVideo" autoplay muted playsinline oncontextmenu="return false;">
              </video>
              <canvas id="myMjpeg" width="600px" height="400px" v-show="!!!this.showVideo" class="axVideoMJpeg">
              </canvas>
            </div>
            <div id="osdBound" class="osdBound">
              <div id="targetOSDTime" class="targetOSDTime" v-show="this.formOverlay.overlay_attr[this.chn_cur].time.enable">
                <canvas id="osdTime" class="osdTime" width="600px" height="400px" v-show="this.formOverlay.overlay_attr[this.chn_cur].time.enable">
                </canvas>
              </div>
              <div id="targetOSDLogo" class="targetOSDLogo" v-show="this.formOverlay.overlay_attr[this.chn_cur].logo.enable">
                <canvas id="osdLogo" class="osdLogo" width="600px" height="400px" v-show="this.formOverlay.overlay_attr[this.chn_cur].logo.enable">
                </canvas>
              </div>
              <div id="targetOSDChannel" class="targetOSDChannel" v-show="this.formOverlay.overlay_attr[this.chn_cur].channel.enable">
                <canvas id="osdChannel" class="osdChannel" width="600px" height="400px" v-show="this.formOverlay.overlay_attr[this.chn_cur].channel.enable">
                </canvas>
              </div>
              <div id="targetOSDLocation" class="targetOSDLocation" v-show="this.formOverlay.overlay_attr[this.chn_cur].location.enable">
                <canvas id="osdLocation" class="osdLocation" width="600px" height="400px" v-show="this.formOverlay.overlay_attr[this.chn_cur].location.enable">
                </canvas>
              </div>
              <div id="targetOSDPrivacy" class="targetOSDPrivacy" v-show="this.formOverlay.overlay_attr[this.chn_cur].privacy.enable">
                <canvas id="osdPrivacy" class="osdPrivacy" width="600px" height="400px" v-show="this.formOverlay.overlay_attr[this.chn_cur].privacy.enable">
                </canvas>
              </div>
              <div id="targetOSDPrivacyPt0" class="targetOSDPrivacyPt0" v-show="this.formOverlay.overlay_attr[this.chn_cur].privacy.enable">
              </div>
              <div id="targetOSDPrivacyPt1" class="targetOSDPrivacyPt1" v-show="this.formOverlay.overlay_attr[this.chn_cur].privacy.enable">
              </div>
              <div id="targetOSDPrivacyPt2" class="targetOSDPrivacyPt2" v-show="this.formOverlay.overlay_attr[this.chn_cur].privacy.enable && this.formOverlay.overlay_attr[this.chn_cur].privacy.type != 0">
              </div>
              <div id="targetOSDPrivacyPt3" class="targetOSDPrivacyPt3" v-show="this.formOverlay.overlay_attr[this.chn_cur].privacy.enable && this.formOverlay.overlay_attr[this.chn_cur].privacy.type != 0">
                <el-select class="Camera" v-model="formOverlay.src_id" size="mini" @change="onChangeSrcID" :disabled="dual_mode == 0">
              </div>
            </div>
          </div>
        </el-aside>
        <el-main class="osdSetting">
          <div>
            <el-form label-width="88px">
              <el-form-item label="Camera:">
                <el-select class="Camera" v-model="formOverlay.src_id" size="mini" @change="onChangeSrcID" :disabled="dual_mode == 0">
                  <el-option v-for="item in Array.from({ length: 2 }, (item, index) => index)" :key="item" :label="'' + item" :value="item"></el-option>
                </el-select>
              </el-form-item>
              <el-form-item label="视频通道:">
                <el-select class="chnVideo" v-model="chn_cur" size="mini" @change="changeStream">
                  <el-option v-for="item in streamOptions[this.formOverlay.src_id]" :key="item.label" :label="item.label" :value="item.id"></el-option>
                </el-select>
              </el-form-item>
            </el-form>
            <el-divider></el-divider>
            <el-tabs tab-position="left" style="height: 200px;">
              <el-tab-pane label="时间标题">
                <el-form ref="formOverlayRef" :rules="formOverlayRules" :model="formOverlay.overlay_attr[chn_cur]" label-width="60px">
                  <el-form-item label="显示：">
                    <el-switch v-model="formOverlay.overlay_attr[chn_cur].time.enable"></el-switch>
                  </el-form-item>
                  <el-form-item label="颜色：">
                    <el-select v-model="formOverlay.overlay_attr[chn_cur].time.color" size="mini" :disabled="!formOverlay.overlay_attr[chn_cur].time.enable">
                      <el-option v-for="item in colorOptions" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                    </el-select>
                  </el-form-item>
                </el-form>
              </el-tab-pane>
              <el-tab-pane label="位置标题">
                <el-form ref="formOverlayRef" :rules="formOverlayRules" :model="formOverlay.overlay_attr[chn_cur]" label-width="60px">
                  <el-form-item label="显示：">
                    <el-switch v-model="formOverlay.overlay_attr[chn_cur].location.enable"></el-switch>
                  </el-form-item>
                  <el-form-item label="颜色：">
                    <el-select v-model="formOverlay.overlay_attr[chn_cur].location.color" size="mini" :disabled="!formOverlay.overlay_attr[chn_cur].location.enable">
                      <el-option v-for="item in colorOptions" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                    </el-select>
                  </el-form-item>
                  <el-form-item label="内容：">
                    <el-input v-model="formOverlay.overlay_attr[chn_cur].location.text" size="mini" maxlength="32" :disabled="!formOverlay.overlay_attr[chn_cur].location.enable" @input="onLocationTextChanged"></el-input>
                  </el-form-item>
                </el-form>
              </el-tab-pane>
              <el-tab-pane label="图标标题">
                <el-form ref="formOverlayRef" :rules="formOverlayRules" :model="formOverlay.overlay_attr[chn_cur]" label-width="60px">
                  <el-form-item label="显示：">
                    <el-switch v-model="formOverlay.overlay_attr[chn_cur].logo.enable"></el-switch>
                  </el-form-item>
                </el-form>
              </el-tab-pane>
              <el-tab-pane label="通道标题">
                <el-form ref="formOverlayRef" :rules="formOverlayRules" :model="formOverlay.overlay_attr[chn_cur]" label-width="60px">
                  <el-form-item label="显示：">
                    <el-switch v-model="formOverlay.overlay_attr[chn_cur].channel.enable"></el-switch>
                  </el-form-item>
                  <el-form-item label="颜色：">
                    <el-select v-model="formOverlay.overlay_attr[chn_cur].channel.color" size="mini" :disabled="!formOverlay.overlay_attr[chn_cur].channel.enable">
                      <el-option v-for="item in colorOptions" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                    </el-select>
                  </el-form-item>
                  <el-form-item label="内容：">
                    <el-input v-model="formOverlay.overlay_attr[chn_cur].channel.text" size="mini" maxlength="32" :disabled="!formOverlay.overlay_attr[chn_cur].channel.enable" @input="onChannelTextChanged"></el-input>
                  </el-form-item>
                </el-form>
              </el-tab-pane>
              <el-tab-pane label="隐私遮挡">
                <el-form ref="formOverlayRef" :rules="formOverlayRules" :model="formOverlay.overlay_attr[chn_cur]" label-width="60px">
                  <el-form-item label="显示：">
                    <el-switch v-model="formOverlay.overlay_attr[chn_cur].privacy.enable"></el-switch>
                  </el-form-item>
                  <el-form-item label="类型：">
                    <el-select v-model="formOverlay.overlay_attr[chn_cur].privacy.type" size="mini" :disabled="!formOverlay.overlay_attr[chn_cur].privacy.enable">
                      <el-option v-for="item in privacyTypeOptions" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                    </el-select>
                  </el-form-item>
                  <el-form-item label="线宽：" v-show="this.formOverlay.overlay_attr[chn_cur].privacy.type != this.OSD_PRIVACY_TYPE_MOSA ">
                    <el-select v-model="formOverlay.overlay_attr[chn_cur].privacy.linewidth" size="mini" :disabled="!formOverlay.overlay_attr[chn_cur].privacy.enable ">
                      <el-option v-for="item in Array.from({ length: 5 }, (item, index) => index + 1)" :key="item" :label="'' + item" :value="item"> </el-option>
                    </el-select>
                  </el-form-item>
                  <el-form-item label="大小：" v-show="this.formOverlay.overlay_attr[chn_cur].privacy.type == this.OSD_PRIVACY_TYPE_MOSA">
                    <el-select v-model="formOverlay.overlay_attr[chn_cur].privacy.linewidth" size="mini" :disabled="!formOverlay.overlay_attr[chn_cur].privacy.enable ">
                      <el-option v-for="item in Array.from({ length: 5 }, (item, index) => index + 1)" :key="item" :label="'' + item" :value="item"> </el-option>
                    </el-select>
                  </el-form-item>
                  <el-form-item label="颜色：" v-show="this.formOverlay.overlay_attr[chn_cur].privacy.type != this.OSD_PRIVACY_TYPE_MOSA">
                    <el-select v-model="formOverlay.overlay_attr[chn_cur].privacy.color" size="mini" :disabled="!formOverlay.overlay_attr[chn_cur].privacy.enable ">
                      <el-option v-for="item in colorOptions" :key="item.label" :label="item.label" :value="item.value"> </el-option>
                    </el-select>
                  </el-form-item>
                  <el-form-item label="填充：" v-show="this.formOverlay.overlay_attr[chn_cur].privacy.type!= this.OSD_PRIVACY_TYPE_LINE && this.formOverlay.overlay_attr[chn_cur].privacy.type != this.OSD_PRIVACY_TYPE_MOSA">
                    <el-switch v-model="formOverlay.overlay_attr[chn_cur].privacy.solid" :disabled="!formOverlay.overlay_attr[chn_cur].privacy.enable"></el-switch>
                  </el-form-item>
                </el-form>
              </el-tab-pane>
            </el-tabs>
          </div>
        </el-main>
      </el-container>
      <el-footer class="overlayFooter">
        <el-divider class="divider"></el-divider>
        <el-button type="primary" @click="onSubmit" size="mini">修改</el-button>
        <el-button type="primary" @click="onFresh" size="mini">刷新</el-button>
      </el-footer>
    </el-container>
  </div>
</template>

<script>
import Wfs from '../../plugins/wfs-min.js'
export default {
  props: ["dual_mode"],
  data() {
    return {
      blockOptions: [8, 16, 64],
      rotationOptions: [0, 90, 180, 270],
      colorOptions: [
        {
          label: '白色',
          value: '0xFFFFFF'
        },
        {
          label: '黑色',
          value: '0x000000'
        },
        {
          label: '绿色',
          value: '0x00FF00'
        },
        {
          label: '红色',
          value: '0xFF0000'
        },
        {
          label: '蓝色',
          value: '0x0000FF'
        },
        {
          label: '黄色',
          value: '0xFFFF00'
        },
        {
          label: '紫色',
          value: '0xFF00FF'
        }
      ],
      cameraOptions: [0, 1],
      streamOptions: [
        [
          {
            label: '主码流0',
            value: 0,
            mediaType: 'H264Raw',
            mediaFPS: 25
          },
          {
            label: '子码流1',
            value: 1,
            mediaType: 'H264Raw',
            mediaFPS: 25
          }
        ],
        [
          {
            label: '主码流0',
            value: 0,
            mediaType: 'H264Raw',
            mediaFPS: 25
          },
          {
            label: '子码流1',
            value: 1,
            mediaType: 'H264Raw',
            mediaFPS: 25
          }
        ]
      ],
      OSD_PRIVACY_TYPE_LINE: 0,
      OSD_PRIVACY_TYPE_RECT: 1,
      OSD_PRIVACY_TYPE_POLY: 2,
      OSD_PRIVACY_TYPE_MOSA: 3,
      privacyTypeOptions: [
        {
          label: '直线',
          value: 0
        },
        {
          label: '矩形',
          value: 1
        },
        {
          label: '四边形',
          value: 2
        },
        {
          label: '马赛克',
          value: 3
        }
      ],
      showVideo: true,
      videoDispW: 0,
      videoDispH: 0,
      chn_cur: 0,
      max_try_num: 3,
      OSD_ALIGN_TYPE_LEFT_TOP: 0,
      OSD_ALIGN_TYPE_RIGHT_TOP: 1,
      OSD_ALIGN_TYPE_LEFT_BOTTOM: 2,
      OSD_ALIGN_TYPE_RIGHT_BOTTOM: 3,
      mjpeg_cxt: {
        isDrawImg: true,
        mjpeg_w: 0,
        mjpeg_h: 0,
        sizeOptionInd: 0,
        getObjectFitSize: this.getObjectFitSize,
        timer_draw: undefined
      },
      formOverlay: {
        src_id: 0,
        overlay_attr: [
          {
            video: {
              id: 0,
              width: 2688,
              height: 1520,
            },
            time: {
              enable: true,
              color: '0xFFFFFF',
              fontsize: 0,
              align: this.OSD_ALIGN_TYPE_LEFT_TOP,
              rect: [0.02, 0.02, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            },
            logo: {
              enable: true,
              align: this.OSD_ALIGN_TYPE_RIGHT_BOTTOM,
              rect: [0.88, 0.88, 0.1, 0.1] // x/W,y/H,w/W,h/H  percent
            },
            channel: {
              enable: false,
              text: "channel",
              color: '0xFFFFFF',
              fontsize: 0,
              align: this.OSD_ALIGN_TYPE_RIGHT_TOP,
              rect: [0.56, 0.02, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            },
            location: {
              enable: false,
              text: "location",
              color: '0xFFFFFF',
              fontsize: 0,
              align: this.OSD_ALIGN_TYPE_LEFT_BOTTOM,
              rect: [0.02, 0.88, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            },
            privacy: {
              enable: false,
              type: 0,
              color: '0xFFFFFF',
              linewidth: 1,
              solid: false,
              rect: [0.02, 0.88, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            }
          },
          {
            video: {
              id: 0,
              width: 720,
              height: 576,
            },
            time: {
              enable: true,
              color: '0xFFFFFF',
              fontsize: 0,
              format: 0,
              rect: [0.02, 0.02, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            },
            logo: {
              enable: true,
              rect: [0.88, 0.88, 0.1, 0.1] // x/W,y/H,w/W,h/H  percent
            },
            channel: {
              enable: false,
              text: "channel",
              color: '0xFFFFFF',
              fontsize: 0,
              rect: [0.56, 0.02, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            },
            location: {
              enable: false,
              text: "location",
              color: '0xFFFFFF',
              fontsize: 0,
              rect: [0.02, 0.88, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            },
            privacy: {
              enable: false,
              type: 0,
              color: '0xFFFFFF',
              linewidth: 2,
              solid: false,
              rect: [0.02, 0.88, 0.4, 0.06] // x/W,y/H,w/W,h/H  percent
            }
          }
        ]
      },
      formOverlayRules: {
      }
    }
  },
  created() {
    console.log('overlay created ++')
    this.getInfo()
    console.log('overlay created --')
  },
  mounted() {
    console.log('overlay mounted ++')
    this.getVideoInfo()
    this.$nextTick(function () {
      this.resize_canvas()
    })

    window.addEventListener('visibilitychange', () => {
      if (this.showVideo && this.wfsObj)
        this.wfsObj.playerSeekNow();
    })
    window.addEventListener('resize', () => {
      if (this.wfsObj && window.visibilityState === 'visible')
        this.resize_canvas();
    })

    // this.startPreview()
    this.dragElement(targetOSDTime, { bounding: osdBound, index: 0, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDChannel, { bounding: osdBound, index: 1, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDLogo, { bounding: osdBound, index: 2, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDLocation, { bounding: osdBound, index: 3, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDPrivacy, { bounding: osdBound, index: 4, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDPrivacyPt0, { bounding: osdBound, index: 5, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDPrivacyPt1, { bounding: osdBound, index: 6, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDPrivacyPt2, { bounding: osdBound, index: 7, onEnd: this.onOsdDragEnd })
    this.dragElement(targetOSDPrivacyPt3, { bounding: osdBound, index: 8, onEnd: this.onOsdDragEnd })
    console.log('overlay mounted --')
  },
  destroyed() {
    this.stopPreview()
  },
  methods: {
    startPreview() {
      this.startPlay()
    },
    stopPreview() {
      this.isForceStop = true
      this.stop()
    },
    async startPlay() {
      var is_play_ok = false
      for (var i = 0; i < this.max_try_num; i++) {
        if (is_play_ok) {
          break
        }

        try {
          await this.changeStream(this.chn_cur + '', false, false)
          this.play()
          is_play_ok = true
        } catch (err) {
          is_play_ok = false
          console.log('startPlay catch except: ' + err)
        }
      }

      if (!is_play_ok) {
        this.$message.error('网络请求码流失败')
      }
    },
    play() {
      console.log('play ++')
      try {
        this.stop()
        const tokenStr = window.sessionStorage.getItem('token')
        const myVideo = document.getElementById('myVideo')
        const myMjpeg = document.getElementById('myMjpeg')

        let mediaType = this.streamOptions[this.formOverlay.src_id][this.chn_cur].mediaType
        let mediaFPS = this.streamOptions[this.formOverlay.src_id][this.chn_cur].mediaFPS
        let extra = this.mjpeg_cxt

        this.showVideo = Wfs.canPlayByVideo(mediaType);
        if (Wfs.isSupported()) {
          let config = {
            wsMinPacketInterval: 2000, // 最大接收数据超时时间，单位毫秒
            wsMaxPacketInterval: 8000,  // 检查超时间隔，单位毫秒
            fps: mediaFPS
          };

          let media = {
            video: myVideo,
            ctx: myMjpeg,
          }

          this.wfsObj = new Wfs(config)
          let id = this.formOverlay.src_id

          this.wfsObj.attachMedia(media, 'ch' + id, mediaType, 'ws/preview_' + id + '?token=' + tokenStr, extra)
          this.wfsObj.on(Wfs.Events.ERROR, (eventName, data) => this.handleWfsEvent(eventName, data))

          if (mediaType == 'MJPEG') {
            this.resize_canvas()
          }

        } else {
          console.log('wfs is not supported')
        }
      } catch (err) {
        console.log('play catch except: ' + err)
      }
      console.log('play --')
    },
    stop() {
      console.log('stop ++')
      if (this.wfsObj != null) {
        this.wfsObj.destroy()
        this.wfsObj = null
      }
      console.log('stop --')
    },
    handleWfsEvent(eventName, data) {
      try {
        if (data.fatal) {
          this.isError = true
          console.warn('Fatal error :' + data.details)
          switch (data.type) {
            case Wfs.ErrorTypes.MEDIA_ERROR:
              console.log('a media error occurred')
              break
            case Wfs.ErrorTypes.NETWORK_ERROR:
              console.log('a network error occurred')
              break
            default:
              console.log('an unrecoverable error occurred')
              break
          }
          this.play()
        }
      } catch (error) {
        console.log('handleWfsEvent catch except: ' + error.description)
      }
    },
    resize_canvas() {
      try {
        var canvas = document.getElementById('myMjpeg')
        if (canvas) {
          this.mjpeg_cxt.mjpeg_w = this.$refs.osdVideoRef.width
          this.mjpeg_cxt.mjpeg_h = this.$refs.osdVideoRef.height
          this.mjpeg_cxt.mjpeg_w = 600
          this.mjpeg_cxt.mjpeg_h = 400
          canvas.setAttribute("width", this.mjpeg_cxt.mjpeg_w)
          canvas.setAttribute("height", this.mjpeg_cxt.mjpeg_h)

          console.log('mjpeg_cxt canvas_w=' + this.mjpeg_cxt.mjpeg_w + ', canvas_h =' + this.mjpeg_cxt.mjpeg_h)

          this.mjpeg_cxt.isDrawImg = false
          if (this.mjpeg_cxt.timer_draw) {
            clearInterval(this.mjpeg_cxt.timer_draw)
          }
          this.mjpeg_cxt.timer_draw = setInterval(() => {
            this.mjpeg_cxt.isDrawImg = true
            clearInterval(this.mjpeg_cxt.timer_draw)
          }, 300)
        }
      } catch (error) {
        console.log('resize_canvas except: ' + error)
      }
    },
    async onSubmit() {
      try {
        var _uri = 'setting/overlay'
        console.log('setting/overlay' + this.formOverlay)
        const { data: res } = await this.$http.post(_uri, this.formOverlay)
        console.log('overlay post return: ', res)
        if (res.meta.status === 200) {
          this.$message.success('修改成功')
        } else {
          this.$message.success('修改失败')
        }
      } catch (error) {
        this.$message.error('修改失败')
      }
    },
    async onFresh() {
      console.log('onFresh ++')
      this.getInfo()
      console.log('onFresh --')
    },
    async getInfo() {
      console.log('getInfo ++')
      try {
        var _uri = 'setting/overlay'
        const { data: res } = await this.$http.get(_uri, { params: { src_id: this.formOverlay.src_id } })
        console.log('overlay get return: ', res)
        if (res.meta.status === 200) {
          this.formOverlay.overlay_attr = res.data.overlay_attr
          console.log('overlay get srcid: ', this.formOverlay.src_id)
          this.resize_canvas()
          this.drawOSDTime()
          this.drawOSDLogo()
          this.drawOSDChannel()
          this.drawOSDLocation()
          this.drawOSDPrivacy()
        }
      } catch (error) {
        console.log('获取信息失败' + error)
        this.$message.error('获取信息失败')
      }
      console.log('getInfo --')
    },
    async changeStream(value, replay = true, changeData = true) {
      console.log('changeStream ++')
      // request http to send this configure to server
      const steamInd = Number(value)
      console.log('change stream to ' + steamInd)

      const { data: res } = await this.$http.post('preview/stream', { src_id: this.formOverlay.src_id, stream: steamInd })
      console.log('post stream return: ', res)
      if (!res || res.meta.status !== 200) {
        return this.$message.error('设置码流失败')
      }
      if (changeData) {
        this.chn_cur = steamInd
        window.localStorage.setItem('stream_' + this.formOverlay.src_id, this.streamOptions[this.formOverlay.src_id][steamInd].id)
      }
      if (replay) {
        this.play()
        this.drawOSDTime()
        this.drawOSDLogo()
        this.drawOSDChannel()
        this.drawOSDLocation()
        this.drawOSDPrivacy()
      }
      console.log('changeStream --')
    },
    async getVideoInfo() {
      try {
        const { data: res } = await this.$http.get('preview/info')
        console.log('preview info get return: ', res)
        if (res.meta.status === 200) {
          this.sns_num = res.data.info.sns_num

          /* stream capability(dual sensor) */
          this.streamOptions = [[], []]
          for (var i = 0; i < res.data.info.stream0_list.length; i++) {
            var stream_id = res.data.info.stream0_list[i]
            var _item = {
              id: stream_id,
              label: (0 === stream_id) ? ('主码流0') : ('子码流' + stream_id + ''),
              icon: 'el-icon-full-screen',
              mediaType: this.getMediatype(res.data.info.sns0_codec[i]),
              mediaFPS: res.data.info.sns0_video_fps[i]
            }
            this.streamOptions[0].push(_item)
          }
          if (this.sns_num > 1) {
            for (var i = 0; i < res.data.info.stream1_list.length; i++) {
              var stream_id = res.data.info.stream1_list[i]
              var _item = {
                id: stream_id,
                label: (0 === stream_id) ? ('主码流0') : ('子码流' + stream_id + ''),
                icon: 'el-icon-full-screen',
                mediaType: this.getMediatype(res.data.info.sns1_codec[i]),
                mediaFPS: res.data.info.sns1_video_fps[i]

              }
              this.streamOptions[1].push(_item)
            }
          }
          this.mjpeg_cxt.sizeOptionInd = 0
          this.mjpeg_cxt.getObjectFitSize = this.getObjectFitSize

          this.startPreview()
        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    },
    getMediatype(codec_type) {
      if (codec_type === 0) {
        return 'H264Raw';
      } else if (codec_type === 1) {
        return 'MJPEG';
      } else if (codec_type === 2) {
        return 'H265Raw';
      }

      return 'H264Raw';
    },
    getObjectFitSize(
      type = 0,
      containerWidth,
      containerHeight,
      imgWidth,
      imgHeight) {
      let radio = 1, sx = 0, sy = 0, swidth = imgWidth, sheight = imgHeight,
        x = 0, y = 0, width = containerWidth, height = containerHeight
      let cWHRatio = containerWidth / containerHeight
      let iWHRatio = imgWidth / imgHeight
      if (type === 0) { // fill
        if (iWHRatio >= cWHRatio) {
          radio = containerHeight / imgHeight
          sx = (imgWidth - containerWidth / radio) / 2
          swidth = containerWidth / radio
          sheight = imgHeight
        } else {
          radio = containerWidth / imgWidth
          sy = (imgHeight - containerHeight / radio) / 2
          swidth = imgWidth
          sheight = containerHeight / radio
        }
      } else if (type === 1) { // contain
        if (iWHRatio >= cWHRatio) {
          radio = containerWidth / imgWidth
          y = (containerHeight - imgHeight * radio) / 2
          height = imgHeight * radio;
        } else {
          radio = containerHeight / imgHeight
          x = (containerWidth - imgWidth * radio) / 2
          width = imgWidth * radio
        }
      }
      return {
        sx,
        sy,
        swidth,
        sheight,
        x,
        y,
        width,
        height,
      }
    },
    onChangeSrcID() {
      this.getInfo()
      this.startPreview()
    },
    onChannelTextChanged() {
      this.drawOSDChannel()
    },
    onLocationTextChanged() {
      this.drawOSDLocation()
    },
    drawOSDTime() {
      console.log('drawOSDTime ..')
      try {
        var bounding = document.getElementById("osdBound").getBoundingClientRect()
        var x = this.formOverlay.overlay_attr[this.chn_cur].time.rect[0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y = this.formOverlay.overlay_attr[this.chn_cur].time.rect[1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height
        var w = this.formOverlay.overlay_attr[this.chn_cur].time.rect[2] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var h = this.formOverlay.overlay_attr[this.chn_cur].time.rect[3] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height

        x = x < bounding.width ? x : (bounding.width - 2)
        y = y < bounding.height ? y : (bounding.height - 2)
        console.log("time pos:  " + "w=" + w + " h = " + h + "boundW= " + bounding.width + "boundH= " + bounding.height)
        w = (x + w) <= bounding.width ? w : (bounding.width - x)
        h = (y + h) <= (bounding.height) ? h : (bounding.height - h)

        var eleTarget = document.getElementById("targetOSDTime")
        eleTarget.style.left = x + 'px'
        eleTarget.style.top = y + 'px'
        eleTarget.style.width = w + 'px'
        eleTarget.style.height = h + 'px'
        console.log("time pos: x=" + x + ", y=" + y + ", w=" + w + ", h=" + h)

        var canvas = document.getElementById("osdTime")
        var context = canvas.getContext("2d")
        context.clearRect(0, 0, canvas.width, canvas.height)
        context.font = (h - 8) + 'px Arial Narrow'
        console.log("font=" + context.font)

        context.fillStyle = 'red'
        context.fillText("XXXX-XX-XX XX:XX:XX", 1, h - 8)
      } catch (error) {
        console.log('drawOSDLogo: ' + error)
      }
    },
    drawOSDLogo() {
      console.log('drawOSDLogo ..')
      try {
        var bounding = document.getElementById("osdBound").getBoundingClientRect()
        var x = this.formOverlay.overlay_attr[this.chn_cur].logo.rect[0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y = this.formOverlay.overlay_attr[this.chn_cur].logo.rect[1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height
        var w = this.formOverlay.overlay_attr[this.chn_cur].logo.rect[2] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var h = this.formOverlay.overlay_attr[this.chn_cur].logo.rect[3] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height

        x = x < bounding.width ? x : (bounding.width - 2)
        y = y < bounding.height ? y : (bounding.height - 2)
        w = (x + w) < bounding.width ? w : (bounding.width - x)
        h = (y + h) < bounding.height ? h : (bounding.height - h)

        var eleTarget = document.getElementById("targetOSDLogo")
        eleTarget.style.left = x + 'px'
        eleTarget.style.top = y + 'px'
        eleTarget.style.width = w + 'px'
        eleTarget.style.height = h + 'px'
        console.log("logo pos: x=" + x + ", y=" + y + ", w=" + w + ", h=" + h)
      } catch (error) {
        console.log('drawOSDLogo: ' + error)
      }
    },
    drawOSDLocation() {
      console.log('drawOSDLocation ..')
      try {
        var bounding = document.getElementById("osdBound").getBoundingClientRect()
        var x = this.formOverlay.overlay_attr[this.chn_cur].location.rect[0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y = this.formOverlay.overlay_attr[this.chn_cur].location.rect[1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height
        var w = this.formOverlay.overlay_attr[this.chn_cur].location.rect[2] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var h = this.formOverlay.overlay_attr[this.chn_cur].location.rect[3] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height

        x = x < bounding.width ? x : (bounding.width - 1)
        y = y < bounding.height ? y : (bounding.height - 1)
        w = (x + w) < bounding.width ? w : (bounding.width - x)
        h = (y + h) < bounding.height ? h : (bounding.height - h)

        var eleTarget = document.getElementById("targetOSDLocation")
        eleTarget.style.left = x + 'px'
        eleTarget.style.top = y + 'px'
        eleTarget.style.width = w + 'px'
        eleTarget.style.height = h + 'px'
        console.log("location pos: x=" + x + ", y=" + y + ", w=" + w + ", h=" + h)

        var canvas = document.getElementById("osdLocation")
        var context = canvas.getContext("2d")
        context.clearRect(0, 0, canvas.width, canvas.height)
        context.font = (h - 2) + 'px Arial Narrow'
        context.fillStyle = 'red'
        context.fillText(this.formOverlay.overlay_attr[this.chn_cur].location.text, 1, h - 2)
      } catch (error) {
        console.log('drawOSDLocation: ' + error)
      }
    },
    drawOSDChannel() {
      console.log('drawOSDChannel ..')
      try {
        var bounding = document.getElementById("osdBound").getBoundingClientRect()
        var x = this.formOverlay.overlay_attr[this.chn_cur].channel.rect[0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y = this.formOverlay.overlay_attr[this.chn_cur].channel.rect[1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height
        var w = this.formOverlay.overlay_attr[this.chn_cur].channel.rect[2] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var h = this.formOverlay.overlay_attr[this.chn_cur].channel.rect[3] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height

        x = x < bounding.width ? x : (bounding.width - 2)
        y = y < bounding.height ? y : (bounding.height - 2)
        w = (x + w) < bounding.width ? w : (bounding.width - x)
        h = (y + h) < bounding.height ? h : (bounding.height - h)

        var eleTarget = document.getElementById("targetOSDChannel")
        eleTarget.style.left = x + 'px'
        eleTarget.style.top = y + 'px'
        eleTarget.style.width = w + 'px'
        eleTarget.style.height = h + 'px'
        console.log("channel pos: x=" + x + ", y=" + y + ", w=" + w + ", h=" + h)

        var canvas = document.getElementById("osdChannel")
        var context = canvas.getContext("2d")
        context.clearRect(0, 0, canvas.width, canvas.height)
        context.font = (h - 2) + 'px Arial Narrow'
        context.fillStyle = 'red'
        var txt = this.formOverlay.overlay_attr[this.chn_cur].channel.text
        var txt_w = context.measureText(txt).width
        context.fillText(txt, w - txt_w, h - 2)
        console.log("channel str width: " + txt_w)
      } catch (error) {
        console.log('drawOSDLocation: ' + error)
      }
    },
    drawOSDPrivacy() {
      console.log('drawOSDPrivacy ..')
      try {
        var bounding = document.getElementById("osdBound").getBoundingClientRect()
        var x0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height
        var x1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height
        var x2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height
        var x3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][0] / this.formOverlay.overlay_attr[this.chn_cur].video.width * bounding.width
        var y3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][1] / this.formOverlay.overlay_attr[this.chn_cur].video.height * bounding.height

        x0 = x0 < bounding.width ? x0 : (bounding.width - 2)
        y0 = y0 < bounding.height ? y0 : (bounding.height - 2)
        x1 = x1 < bounding.width ? x1 : (bounding.width - 2)
        y1 = y1 < bounding.height ? y1 : (bounding.height - 2)
        x2 = x2 < bounding.width ? x2 : (bounding.width - 2)
        y2 = y2 < bounding.height ? y2 : (bounding.height - 2)
        x3 = x3 < bounding.width ? x3 : (bounding.width - 2)
        y3 = y3 < bounding.height ? y3 : (bounding.height - 2)


        var x, y, w, h
        if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_LINE) {
          var xs = [x0, x1]
          var ys = [y0, y1]
          x = Math.min.apply(null, xs)
          y = Math.min.apply(null, ys)
          w = Math.max.apply(null, xs) - x
          h = Math.max.apply(null, ys) - y
        } else if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_POLY) {
          var xs = [x0, x1, x2, x3]
          var ys = [y0, y1, y2, y3]
          x = Math.min.apply(null, xs)
          y = Math.min.apply(null, ys)
          w = Math.max.apply(null, xs) - x
          h = Math.max.apply(null, ys) - y
        } else {
          var xs = [x0, x1, x2, x3]
          var ys = [y0, y1, y2, y3]
          x = x0
          y = y0
          w = x2 - x0
          h = y2 - y0
        }
        var eleTarget = document.getElementById("targetOSDPrivacy")
        eleTarget.style.left = x + 'px'
        eleTarget.style.top = y + 'px'
        eleTarget.style.width = w + 'px'
        eleTarget.style.height = h + 'px'
        console.log("privacy pos: x=" + x + ", y=" + y + ", w=" + w + ", h=" + h)

        var canvas = document.getElementById("osdPrivacy")
        canvas.setAttribute("width", w)
        canvas.setAttribute("height", h)
        var context = canvas.getContext("2d")
        context.clearRect(0, 0, canvas.width, canvas.height)
        context.fillStyle = 'red'
        if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_LINE) {
          var eleTarget = document.getElementById("targetOSDPrivacyPt0")
          eleTarget.style.left = (x0 - 2) + 'px'
          eleTarget.style.top = (y0 - 2) + 'px'
          eleTarget = document.getElementById("targetOSDPrivacyPt1")
          eleTarget.style.left = (x1 - 2) + 'px'
          eleTarget.style.top = (y1 - 2) + 'px'
        } else if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_POLY) {
          var eleTarget = document.getElementById("targetOSDPrivacyPt0")
          eleTarget.style.left = (x0 - 2) + 'px'
          eleTarget.style.top = (y0 - 2) + 'px'
          eleTarget = document.getElementById("targetOSDPrivacyPt1")
          eleTarget.style.left = (x1 - 2) + 'px'
          eleTarget.style.top = (y1 - 2) + 'px'
          eleTarget = document.getElementById("targetOSDPrivacyPt2")
          eleTarget.style.left = (x2 - 2) + 'px'
          eleTarget.style.top = (y2 - 2) + 'px'
          eleTarget = document.getElementById("targetOSDPrivacyPt3")
          eleTarget.style.left = (x3 - 2) + 'px'
          eleTarget.style.top = (y3 - 2) + 'px'
        } else {
          var eleTarget = document.getElementById("targetOSDPrivacyPt0")
          eleTarget.style.left = (x - 2) + 'px'
          eleTarget.style.top = (y - 2) + 'px'
          eleTarget = document.getElementById("targetOSDPrivacyPt1")
          eleTarget.style.left = (x + w - 2) + 'px'
          eleTarget.style.top = (y - 2) + 'px'
          eleTarget = document.getElementById("targetOSDPrivacyPt2")
          eleTarget.style.left = (x + w - 2) + 'px'
          eleTarget.style.top = (y + h - 2) + 'px'
          eleTarget = document.getElementById("targetOSDPrivacyPt3")
          eleTarget.style.left = (x - 2) + 'px'
          eleTarget.style.top = (y + h - 2) + 'px'
        }

      } catch (error) {
        console.log('drawOSDPrivacy: ' + error)
      }
    },
    onOsdDragEnd(target, index) {
      var bounding = document.getElementById("osdBound").getBoundingClientRect()
      var x = parseInt(parseInt(target.style.left) / bounding.width * this.formOverlay.overlay_attr[this.chn_cur].video.width)
      var y = parseInt(parseInt(target.style.top) / bounding.height * this.formOverlay.overlay_attr[this.chn_cur].video.height)
      var w = parseInt(parseInt(target.style.width) / bounding.width * this.formOverlay.overlay_attr[this.chn_cur].video.width)
      var h = parseInt(parseInt(target.style.height) / bounding.height * this.formOverlay.overlay_attr[this.chn_cur].video.height)
      x = x < 0 ? 0 : x
      y = y < 0 ? 0 : y
      console.log("osd bound in web top=" + target.style.top + ",left=" + target.style.left + ",width=" + target.style.width + ",height=" + target.style.height)
      console.log("osd bound in video x=" + x + ",y=" + y + ",width=" + w + ",height=" + h)
      if (index === 0) {
        this.formOverlay.overlay_attr[this.chn_cur].time.rect = [x, y, w, h]
        console.log("update time rect: " + this.formOverlay.overlay_attr[this.chn_cur].time.rect)
      }
      else if (index === 1) {
        this.formOverlay.overlay_attr[this.chn_cur].channel.rect = [x, y, w, h]
        console.log("update channel rect: " + this.formOverlay.overlay_attr[this.chn_cur].channel.rect)
      }
      else if (index === 2) {
        this.formOverlay.overlay_attr[this.chn_cur].logo.rect = [x, y, w, h]
        console.log("update logo rect: " + this.formOverlay.overlay_attr[this.chn_cur].logo.rect)
      }
      else if (index === 3) {
        this.formOverlay.overlay_attr[this.chn_cur].location.rect = [x, y, w, h]
        console.log("update location rect: " + this.formOverlay.overlay_attr[this.chn_cur].location.rect)
      }
      else if (index === 4) {
        console.log("update privacy rect: " + [x, y, w, h])
        var x0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][0]
        var y0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][1]
        var x1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][0]
        var y1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][1]
        var x2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][0]
        var y2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][1]
        var x3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][0]
        var y3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][1]

        if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_LINE) {
          var xs0 = [x0, x1]
          var ys0 = [y0, y1]
          var xd = x - Math.min.apply(null, xs0)
          var yd = y - Math.min.apply(null, ys0)
          this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0] = [(x0 + xd) < 0 ? 0 : (x0 + xd), (y0 + yd) < 0 ? 0 : (y0 + yd)]
          this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1] = [(x1 + xd) < 0 ? 0 : (x1 + xd), (y1 + yd) < 0 ? 0 : (y1 + yd)]
        } else if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_POLY) {
          var xs0 = [x0, x1, x2, x3]
          var ys0 = [y0, y1, y2, y3]
          var xd = x - Math.min.apply(null, xs0)
          var yd = y - Math.min.apply(null, ys0)
          console.log("xs0 " + xs0 + ", x=" + x + ",xd=" + xd + ",min=" + Math.min.apply(null, xs0))
          console.log("ys0 " + ys0 + ", x=" + y + ",yd=" + yd + ",min=" + Math.min.apply(null, ys0))
          console.log("privacy points old:" + this.formOverlay.overlay_attr[this.chn_cur].privacy.points)
          this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0] = [(x0 + xd) < 0 ? 0 : (x0 + xd), (y0 + yd) < 0 ? 0 : (y0 + yd)]
          this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1] = [(x1 + xd) < 0 ? 0 : (x1 + xd), (y1 + yd) < 0 ? 0 : (y1 + yd)]
          this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2] = [(x2 + xd) < 0 ? 0 : (x2 + xd), (y2 + yd) < 0 ? 0 : (y2 + yd)]
          this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3] = [(x3 + xd) < 0 ? 0 : (x3 + xd), (y3 + yd) < 0 ? 0 : (y3 + yd)]
        } else {
          this.formOverlay.overlay_attr[this.chn_cur].privacy.points = [[x, y], [x + w, y], [x + w, y + h], [x, y + h]]
        }
        this.drawOSDPrivacy()
        console.log("privacy points new:" + this.formOverlay.overlay_attr[this.chn_cur].privacy.points)
      }
      else if (index === 5) {
        // pt0
        this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0] = [x + 2, y + 2]
        if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_RECT || this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_MOSA) {
          var x0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][0]
          var y0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][1]
          var x1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][0]
          var y1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][1]
          var x2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][0]
          var y2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][1]
          var x3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][0]
          var y3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][1]
          if (x0 < x2) {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1] = [x2, y0]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3] = [x0, y2]
          } else {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1] = [x0, y2]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3] = [x2, y0]
          }

        }
        this.drawOSDPrivacy()
      }
      else if (index === 6) {
        // pt1
        this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1] = [x + 2, y + 2]
        if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_RECT || this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_MOSA) {
          var x0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][0]
          var y0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][1]
          var x1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][0]
          var y1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][1]
          var x2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][0]
          var y2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][1]
          var x3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][0]
          var y3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][1]
          if (x1 > x3) {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0] = [x3, y1]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2] = [x1, y3]
          } else {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0] = [x1, y3]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2] = [x3, y1]
          }
        }
        this.drawOSDPrivacy()
      }
      else if (index === 7) {
        // pt2
        this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2] = [x + 2, y + 2]
        if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_RECT || this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_MOSA) {
          var x0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][0]
          var y0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][1]
          var x1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][0]
          var y1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][1]
          var x2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][0]
          var y2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][1]
          var x3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][0]
          var y3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][1]
          if (x0 < x2) {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1] = [x2, y0]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3] = [x0, y2]
          } else {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1] = [x0, y2]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3] = [x2, y0]
          }
        }
        this.drawOSDPrivacy()
      }
      else if (index === 8) {
        // pt3
        this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3] = [x + 2, y + 2]
        if (this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_RECT || this.formOverlay.overlay_attr[this.chn_cur].privacy.type === this.OSD_PRIVACY_TYPE_MOSA) {
          var x0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][0]
          var y0 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0][1]
          var x1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][0]
          var y1 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[1][1]
          var x2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][0]
          var y2 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2][1]
          var x3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][0]
          var y3 = this.formOverlay.overlay_attr[this.chn_cur].privacy.points[3][1]
          if (x1 > x3) {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0] = [x3, y1]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2] = [x1, y3]
          } else {
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[0] = [x1, y3]
            this.formOverlay.overlay_attr[this.chn_cur].privacy.points[2] = [x3, y1]
          }
        }
        this.drawOSDPrivacy()
      }
    },
    dragElement(eleBar, options) {
      if (!eleBar) {
        return
      }
      var params = {}
      // 默认数据
      var defaults = {
        target: eleBar,
        bounding: window,
        edgeLock: true,
        index: 0,
        onMove: function () { },
        onEnd: function (target, index) { }
      }

      options = options || {}


      for (var key in defaults) {
        if (typeof options[key] != 'undefined') {
          params[key] = options[key]
        } else {
          params[key] = defaults[key]
        }
      }

      // 拖拽元素
      var eleTarget = params.target
      // 限制范围
      var bounding = params.bounding
      var objBounding = bounding

      // 事件类型处理
      var objEventType = {
        start: 'mousedown',
        move: 'mousemove',
        end: 'mouseup'
      }

      if ('ontouchstart' in document) {
        objEventType = {
          start: 'touchstart',
          move: 'touchmove',
          end: 'touchend'
        }
      }

      // 坐标存储数据
      var store = {}
      eleBar.addEventListener(objEventType.start, function (event) {
        // IE 拖拽可能拖不动的处理
        if (!window.WeakMap || typeof document.msHidden != 'undefined') {
          event.preventDefault()
        }
        // 兼顾移动端
        if (event.touches && event.touches.length) {
          event = event.touches[0]
        }
        store.y = event.pageY
        store.x = event.pageX
        store.isMoving = true
        store.top = parseFloat(getComputedStyle(eleTarget).top) || 0
        store.left = parseFloat(getComputedStyle(eleTarget).left) || 0

        if (params.edgeLock === true && bounding) {
          if (bounding === window) {
            objBounding = {
              left: 0,
              top: 0,
              bottom: innerHeight,
              right: Math.min(innerWidth, document.documentElement.clientWidth)
            }
          } else if (bounding.tagName) {
            objBounding = bounding.getBoundingClientRect()
          }

          // 拖拽元素的 bounding 位置
          var objBoundingTarget = eleTarget.getBoundingClientRect()

          // 可移动范围
          store.range = {
            y: [objBounding.top - objBoundingTarget.top, objBounding.bottom - objBoundingTarget.bottom],
            x: [objBounding.left - objBoundingTarget.left, objBounding.right - objBoundingTarget.right]
          }
        }
      })
      document.addEventListener(objEventType.move, function (event) {
        if (store.isMoving) {
          event.preventDefault()
          // 兼顾移动端
          if (event.touches && event.touches.length) {
            event = event.touches[0]
          }

          var distanceY = event.pageY - store.y
          var distanceX = event.pageX - store.x

          // 边界的判断与chuli
          if (params.edgeLock === true && bounding) {
            var minX = Math.min.apply(null, store.range.x)
            var maxX = Math.max.apply(null, store.range.x)
            var minY = Math.min.apply(null, store.range.y)
            var maxY = Math.max.apply(null, store.range.y)

            if (distanceX < minX) {
              distanceX = minX
            } else if (distanceX > maxX) {
              distanceX = maxX
            }

            if (distanceY < minY) {
              distanceY = minY
            } else if (distanceY > maxY) {
              distanceY = maxY
            }
          }

          var top = store.top + distanceY
          var left = store.left + distanceX

          eleTarget.style.top = top + 'px'
          eleTarget.style.left = left + 'px'

          // 回调
          params.onMove(left, top)
        }
      }, {
        passive: false
      })
      document.addEventListener(objEventType.end, function () {
        if (store.isMoving) {
          store.isMoving = false;
          params.onEnd(params.target, params.index)
        }
      })
    }
  }
}
</script>

<style lang="less" scoped>
.preview_container {
  margin: 10px !important;
  width: 600px !important;
  height: 400px !important;
  background-color: #000;

  .videoContainer {
    padding: 0 !important;
    width: 100%;
    height: 100%;
    background-color: #000;
    display: flex;
    flex-direction: column;
    align-content: center;
    align-items: center;

    .axVideo {
      width: 100%;
      height: 100%;
      object-fit: fill !important;
      display: inline-block;
    }

    .axVideoMJpeg {
      width: 100%;
      height: 100%;
      object-fit: fill !important;
      display: inline-block;
    }
  }
}

.osdVideo {
  position: absolute;
  z-index: 10 !important;
  width: 600px;
  height: 400px;
  background-color: black;
}

.osdBound {
  position: absolute;
  width: 600px;
  height: 400px;
  //background-color: antiquewhite;
  z-index: 20 !important;
}

.targetOSDTime {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 120px;
  height: 20px;
  border: 1px solid #FF0000;
  z-index: 10 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDLogo {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 120px;
  height: 60px;
  border: 1px solid #FF0000;
  z-index: 9 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDLocation {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 120px;
  height: 60px;
  border: 1px solid #FF0000;
  z-index: 8 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDChannel {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 120px;
  height: 60px;
  border: 1px solid #FF0000;
  z-index: 7 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDPrivacyPt0 {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 4px;
  height: 4px;
  background-color: #FF0000;
  border: 1px solid #FF0000;
  z-index: 6 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDPrivacyPt1 {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 4px;
  height: 4px;
  background-color: #FF0000;
  border: 1px solid #FF0000;
  z-index: 5 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDPrivacyPt2 {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 4px;
  height: 4px;
  background-color: #FF0000;
  border: 1px solid #FF0000;
  z-index: 4 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDPrivacyPt3 {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 4px;
  height: 4px;
  background-color: #FF0000;
  border: 1px solid #FF0000;
  z-index: 3 !important;
  cursor: pointer;
  overflow: hidden;
}

.targetOSDPrivacy {
  position: absolute;
  margin: 0;
  padding: 0;
  width: 100px;
  height: 200px;
  border: 1px solid #FF0000;
  z-index: 2 !important;
  cursor: pointer;
  overflow: hidden;
}

.osdSetting {
  padding-top: 0 !important;
}

.Camera {
  padding-left: 20px !important;
}

.chnVideo {
  padding-left: 20px !important;
}

.divider {
  margin-top: 10px;
}

.el-form-item {
  margin-bottom: 0px;
}

.el-input-number {
  width: 120px;
}

.el-select {
  width: 100px;
}

.inline {
  display: inline-block;
}

.overlayFooter {
  margin-left: 10px !important;
  padding-left: 0px !important;
}
</style>
