<template>
  <div>
    <el-form ref="formCameraRef" :rules="formCameraRules" :model="formCamera" label-width="100px" size="mini">
      <el-form-item label="Camera:">
        <el-select v-model="formCamera.src_id" @change="onChangeSrcID" :disabled="dual_mode == 0">
          <el-option v-for="item in Array.from({ length: 2 }, (item, index) => index)" :key="item" :label="'' + item"
            :value="item"></el-option>
        </el-select>
      </el-form-item>
      <el-form-item label="开启抓拍:" v-show="formCamera.camera_attr.capture_enable">
        <el-switch v-model="formCamera.camera_attr.capture"></el-switch>
      </el-form-item>
      <el-form-item label="工作模式:" v-show="formCamera.camera_attr.switch_work_mode_enable">
        <el-select v-model="formCamera.camera_attr.sns_work_mode">
          <el-option v-for="item in sns_mode_options" :key="item.label" :label="item.label" :value="item.value"> </el-option>
        </el-select>
      </el-form-item>
      <el-form-item label="相机帧率:" v-show="formCamera.camera_attr.switch_PN_mode_enable">
         <el-select v-model="formCamera.camera_attr.framerate">
          <el-option v-for="item in framerate_options" :key="item.label" :label="item.label" :value="item.value">
         </el-option>
        </el-select>
      </el-form-item>
      <el-form-item label="日夜模式:">
        <el-select v-model="formCamera.camera_attr.daynight">
          <el-option v-for="item in daynight_options" :key="item.label" :label="item.label" :value="item.value">
          </el-option>
        </el-select>
      </el-form-item>
      <el-form-item label="旋转:" v-show="formCamera.camera_attr.switch_rotation_enable">
        <el-select v-model="formCamera.camera_attr.rotation">
          <el-option v-for="item in rotation_options" :key="item.label" :label="item.label" :value="item.value"> </el-option>
        </el-select>
      </el-form-item>
      <el-form label-width="100px">
        <el-row>
          <el-col :span="2">
            <el-form-item label="镜像:" v-show="formCamera.camera_attr.switch_mirror_flip_enable">
              <el-switch v-model="formCamera.camera_attr.mirror"></el-switch>
            </el-form-item>
          </el-col>
          <el-col :span="2">
            <el-form-item label="翻转:" v-show="formCamera.camera_attr.switch_mirror_flip_enable">
              <el-switch v-model="formCamera.camera_attr.flip"></el-switch>
            </el-form-item>
          </el-col>
        </el-row>
      </el-form>

      <!-- el-form-item label="开启NR:">
        <el-switch v-model="formCamera.camera_attr.nr_mode"></el-switch>
      </el-form-item>
      <el-form-item label="开启EIS:">
        <el-switch v-model="formCamera.camera_attr.eis" :disabled="!formCamera.camera_attr.eis_support"></el-switch>
      </el-form-item -->
      <el-form-item>
        <el-divider></el-divider>
        <el-button type="primary" @click="onSubmit">修改</el-button>
      </el-form-item>
    </el-form>
  </div>
</template>

<script>
export default {
  props: ["dual_mode"],
  data() {
    return {
      sns_mode_options: [
        {
          label: 'SDR',
          value: 1
        },
        {
          label: 'HDR',
          value: 2
        }
      ],
      rotation_options: [
        {
          label: '0°',
          value: 0
        },
        {
          label: '90°',
          value: 1
        },
        {
          label: '180°',
          value: 2
        },
        {
          label: '270°',
          value: 3
        }
      ],
      daynight_options: [
        {
          label: '日间模式',
          value: 0
        },
        {
          label: '夜间模式',
          value: 1
        }
      ],
      isp_mode: [
        {
          label: '自动模式',
          value: 1
        },
        {
          label: '手动模式',
          value: 0
        }
      ],
      framerate_options: [
        {
          label: '25',
          value: 25
        },
        {
          label: '30',
          value: 30
        }
      ],
      formCamera: {
        src_id: 0,
        camera_attr: {
          sns_work_mode: 1,
          rotation: 0,
          mirror: false,
          flip: false,
          framerate: 25,
          daynight: 0,
          nr_mode: true,
          eis_support: false,
          eis: false,
          capture: true,
          capture_enable: true,
          switch_work_mode_enable: false,
          switch_PN_mode_enable: false,
          framerate_opts: [
            25,
            30
          ]
        }
      },
      formCameraRules: {
        // pending
      }
    }
  },
  created() {
    console.log('camera++')
    this.getInfo()
  },
  methods: {
    onSubmit() {
      this.$refs.formCameraRef.validate(async valid => {
        if (!valid) return false
        try {
          const { data: res } = await this.$http.post('setting/camera', this.formCamera)
          console.log('camera get return: ', res)
          if (res.meta.status === 200) {
            this.$message.success('修改成功')
          } else {
            this.$message.error('修改失败')
          }
        } catch (error) {
          this.$message.error('修改失败')
        }
        this.getInfo()
      })
    },
    async getInfo() {
      try {
        const { data: res } = await this.$http.get('setting/camera', { params: { src_id: this.formCamera.src_id } })
        console.log('camera get return: ', res)
        if (res.meta.status === 200) {
          this.formCamera.camera_attr.sns_work_mode = res.data.camera_attr.sns_work_mode
          this.formCamera.camera_attr.rotation = res.data.camera_attr.rotation
          this.formCamera.camera_attr.mirror = res.data.camera_attr.mirror
          this.formCamera.camera_attr.flip = res.data.camera_attr.flip
          this.formCamera.camera_attr.daynight = res.data.camera_attr.daynight
          this.formCamera.camera_attr.framerate = res.data.camera_attr.framerate
          this.formCamera.camera_attr.nr_mode = res.data.camera_attr.nr_mode
          this.formCamera.camera_attr.eis_support = res.data.camera_attr.eis_support
          this.formCamera.camera_attr.eis = res.data.camera_attr.eis
          this.formCamera.camera_attr.capture = res.data.camera_attr.capture
          this.formCamera.camera_attr.capture_enable = res.data.camera_attr.capture_enable
          this.formCamera.camera_attr.switch_work_mode_enable = res.data.camera_attr.switch_work_mode_enable
          this.formCamera.camera_attr.switch_PN_mode_enable = res.data.camera_attr.switch_PN_mode_enable
          this.formCamera.camera_attr.switch_mirror_flip_enable = res.data.camera_attr.switch_mirror_flip_enable
          this.formCamera.camera_attr.switch_rotation_enable = res.data.camera_attr.switch_rotation_enable
          this.formCamera.framerate_opts = res.data.framerate_opts

          var fps = []
          for (let index = 0; index < this.formCamera.framerate_opts.length; index++) {
            fps.push({ lable: '' + this.formCamera.framerate_opts[index], value: this.formCamera.framerate_opts[index] })
          }
          this.framerate_options = fps
        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    },
    onChangeSrcID() {
      this.getInfo()
    },
  }
}
</script>

<style lang="less" scoped>
.el-input {
  width: 200px !important;
}

.el-select {
  width: 200px;
}

.el-slider {
  width: 400px;
}

.el-form-item {
  margin-bottom: 10px;
}
</style>
