<template>
  <div class="system-container">
    <el-form label-width="100px" size="mini">
      <el-form-item label="APP名称">
        <el-input v-model="appName" readonly></el-input>
      </el-form-item>
      <el-form-item label="APP版本">
        <el-input v-model="appVersion" readonly></el-input>
      </el-form-item>
      <el-form-item label="SDK版本">
        <el-input v-model="sdkVersion" readonly></el-input>
      </el-form-item>
    </el-form>
  </div>
</template>

<script>
export default {
  data () {
    return {
      appName: '--',
      appVersion: '--',
      sdkVersion: '--'
    }
  },
  created () {
    console.log('system++')
    this.getInfo()
  },
  methods: {
    async getInfo () {
      try {
        const { data: res } = await this.$http.get('setting/system')
        console.log('system get return: ', res)
        if (res.meta.status === 200) {
          this.appName = res.data.appName
          this.appVersion = res.data.appVersion
          this.sdkVersion = res.data.sdkVersion
        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    }
  }
}
</script>

<style lang="less" scoped>
.system-container {
  height: 100%;
}
.el-input {
  width: 300px;
}
</style>
