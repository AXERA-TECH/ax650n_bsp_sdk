<template>
  <div>
    <el-form ref="formImageRef" :rules="formImageRules" :model="formImage" label-width="100px" size="mini">
      <el-form-item label="Camera:">
        <el-select v-model="formImage.src_id" @change="onChangeSrcID" :disabled="img_dual_mode == 0">
          <el-option v-for="item in Array.from({ length: 2 }, (item, index) => index)" :key="item" :label="'' + item"
            :value="item"></el-option>
        </el-select>
      </el-form-item>
      <el-form-item label="图像属性:">
        <el-select v-model="formImage.image_attr.isp_auto_mode">
          <el-option v-for="item in isp_mode" :key="item.label" :label="item.label" :value="item.value">
          </el-option>
        </el-select>
      </el-form-item>
      <el-form-item label="" label-width="90px">
          <el-form-item label="亮度:" class="inline" v-show="formImage.image_attr.isp_auto_mode === 0">
            <el-slider v-model="formImage.image_attr.brightness_val" :min="0" :max="100" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue" size="mini"></el-slider>
          </el-form-item>
          <el-form-item label="饱和度:" class="inline" v-show="formImage.image_attr.isp_auto_mode === 0">
            <el-slider v-model="formImage.image_attr.saturation_val" :min="0" :max="100" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue"  size="mini"></el-slider>
          </el-form-item>
          <el-form-item label="对比度:" class="inline" v-show="formImage.image_attr.isp_auto_mode === 0">
            <el-slider v-model="formImage.image_attr.contrast_val" :min="0" :max="100" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue"  size="mini"></el-slider>
          </el-form-item>
          <el-form-item label="锐度:" class="inline" v-show="formImage.image_attr.isp_auto_mode === 0">
            <el-slider v-model="formImage.image_attr.sharpness_val" :min="0" :max="100" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue"  size="mini"></el-slider>
          </el-form-item>
      </el-form-item>
      <el-form-item label="畸变矫正:" v-show="formImage.ldc_attr.ldc_support">
        <el-switch v-model="formImage.ldc_attr.ldc_enable"></el-switch>
      </el-form-item>
      <el-form-item label="" label-width="90px">
          <el-form-item label="保持幅型比:" class="inline" v-show="formImage.ldc_attr.ldc_support && formImage.ldc_attr.ldc_enable">
            <el-switch v-model="formImage.ldc_attr.aspect"></el-switch>
          </el-form-item>
          <el-form-item label="水平缩放:" class="inline" v-show="formImage.ldc_attr.ldc_support && formImage.ldc_attr.ldc_enable && !formImage.ldc_attr.aspect">
            <el-slider v-model="formImage.ldc_attr.x_ratio" :min="0" :max="100" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue"  size="mini"></el-slider>
          </el-form-item>
          <el-form-item label="垂直缩放:" class="inline" v-show="formImage.ldc_attr.ldc_support && formImage.ldc_attr.ldc_enable && !formImage.ldc_attr.aspect">
            <el-slider v-model="formImage.ldc_attr.y_ratio" :min="0" :max="100" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue"  size="mini"></el-slider>
          </el-form-item>
          <el-form-item label="水平垂直缩放:" class="inline" v-show="formImage.ldc_attr.ldc_support && formImage.ldc_attr.ldc_enable && formImage.ldc_attr.aspect">
            <el-slider v-model="formImage.ldc_attr.xy_ratio" :min="0" :max="100" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue"  size="mini"></el-slider>
          </el-form-item>
          <el-form-item label="矫正强度:" class="inline" v-show="formImage.ldc_attr.ldc_support && formImage.ldc_attr.ldc_enable">
            <el-slider v-model="formImage.ldc_attr.distor_ratio" :min="-10000" :max="10000" :show-tooltip="false" show-input
              @change="onChangeImgAttrValue"  size="mini"></el-slider>
          </el-form-item>
      </el-form-item>
      <el-form-item>
        <el-divider></el-divider>
        <el-button type="primary" @click="onSubmit">修改</el-button>
      </el-form-item>
    </el-form>
  </div>
</template>

<script>
export default {
  props: ["img_dual_mode"],
  data() {
    return {
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
      formImage: {
        src_id: 0,
        image_attr: {
          isp_auto_mode: 0,
          sharpness_val: 10,
          brightness_val: 10,
          contrast_val: 10,
          saturation_val: 10,
        },
        ldc_attr: {
          ldc_support: false,
          ldc_enable: false,
          aspect: false,
          x_ratio: 0,
          y_ratio: 0,
          xy_ratio: 0,
          distor_ratio: 0
        }
      },
      formImageRules: {
        // pending
      }
    }
  },
  created() {
    console.log('image ++')
    this.getInfo()
  },
  methods: {
    onSubmit() {
      this.$refs.formImageRef.validate(async valid => {
        if (!valid) return false
        try {
          const { data: res } = await this.$http.post('setting/image', this.formImage)
          console.log('image get return: ', res)
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
        const { data: res } = await this.$http.get('setting/image', { params: { src_id: this.formImage.src_id } })
        console.log('image get return: ', res)
        if (res.meta.status === 200) {
          this.formImage.image_attr.isp_auto_mode = res.data.image_attr.isp_auto_mode
          this.formImage.image_attr.sharpness_val = res.data.image_attr.sharpness
          this.formImage.image_attr.brightness_val = res.data.image_attr.brightness
          this.formImage.image_attr.contrast_val = res.data.image_attr.contrast
          this.formImage.image_attr.saturation_val = res.data.image_attr.saturation
          this.formImage.ldc_attr.ldc_support = res.data.ldc_attr.ldc_support
          this.formImage.ldc_attr.ldc_enable = res.data.ldc_attr.ldc_enable
          this.formImage.ldc_attr.aspect = res.data.ldc_attr.aspect
          this.formImage.ldc_attr.x_ratio = res.data.ldc_attr.x_ratio
          this.formImage.ldc_attr.y_ratio = res.data.ldc_attr.y_ratio
          this.formImage.ldc_attr.xy_ratio = res.data.ldc_attr.xy_ratio
          this.formImage.ldc_attr.distor_ratio = res.data.ldc_attr.distor_ratio
        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    },
    onChangeSrcID() {
      this.getInfo()
    },
    onChangeImgAttrValue() {
      this.$refs.formImageRef.validate(async valid => {
        if (!valid) return false
        try {
          const { data: res } = await this.$http.post('setting/image', this.formImage)
          console.log('image get return: ', res)
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
.el-form-item{
  margin-bottom: 5px;
}
</style>
