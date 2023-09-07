<template>
  <div>
    <el-form ref="formAudioRef" :rules="formAudioRules" :model="formAudio" label-width="100px" size="mini">
      <el-form-item label="麦克风参数:"/>
      <el-form-item label="音量:" class="inline">
        <el-slider v-model="formAudio.capture_attr.volume_val" :min="0" :max="10" :show-tooltip="false" show-input
          @change="onChangeVolume"></el-slider>
      </el-form-item>
      <el-form-item label="扬声器参数:"/>
      <el-form-item label="音量:" class="inline">
        <el-slider v-model="formAudio.play_attr.volume_val" :min="0" :max="10" :show-tooltip="false" show-input
          @change="onChangeVolume"></el-slider>
      </el-form-item>
    </el-form>
  </div>
</template>

<script>
export default {
  props: ["dual_mode"],
  data() {
    return {
      formAudio: {
        src_id: 0,
        capture_attr: {
          volume_val: 1,
        },
        play_attr: {
          volume_val: 1,
        }

      },
      formAudioRules: {
        // pending
      }
    }
  },
  created() {
    console.log('Audio++')
    this.getInfo()
  },
  methods: {
    onSubmit() {
      this.$refs.formAudioRef.validate(async valid => {
        if (!valid) return false
        try {
          const { data: res } = await this.$http.post('setting/audio', this.formAudio)
          console.log('Audio get return: ', res)
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
        const { data: res } = await this.$http.get('setting/audio', { params: { src_id: this.formAudio.src_id } })
        console.log('Audio get return: ', res)
        if (res.meta.status === 200) {
          this.formAudio.capture_attr.volume_val = res.data.info.capture_attr.volume_val
          this.formAudio.play_attr.volume_val = res.data.info.play_attr.volume_val
        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    },
    onChangeSrcID() {
      this.getInfo()
    },
    onChangeVolume() {
      this.$refs.formAudioRef.validate(async valid => {
        if (!valid) return false
        try {
          const { data: res } = await this.$http.post('setting/audio', this.formAudio)
          console.log('Audio get return: ', res)
          if (res.meta.status === 200) {
            this.$message.success('修改成功')
          } else {
            this.$message.error('修改失败')
          }
        } catch (error) {
          this.$message.error('修改失败')
        }
      })
    }
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
</style>
