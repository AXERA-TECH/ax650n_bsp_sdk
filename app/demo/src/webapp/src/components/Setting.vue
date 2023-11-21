<template>
  <div class="setting">
    <el-container class="seting-container">
      <el-aside width="150px">
        <div class="left-container">
          <el-menu router :default-active="activePath" class="setting_menu" background-color="#4c4c4c" text-color="#fff" active-text-color="#ffd04b">
            <el-menu-item index="/setting/system" @click="saveNavState('/setting/system')" v-show="formData.capInfo.support_page_sys > 0">
              <i class="el-icon-setting"></i>
              <span slot="title">系统信息</span>
            </el-menu-item>
            <el-menu-item index="/setting/camera" @click="saveNavState('/setting/camera')" v-show="formData.capInfo.support_page_cam > 0">
              <i class="el-icon-camera"></i>
              <span slot="title">相机设置</span>
            </el-menu-item>
            <el-menu-item index="/setting/image" @click="saveNavState('/setting/image')" v-show="formData.capInfo.support_page_img > 0">
              <i class="el-icon-set-up"></i>
              <span slot="title">图像设置</span>
            </el-menu-item>
            <el-menu-item index="/setting/ai" @click="saveNavState('/setting/ai')" v-show="formData.capInfo.support_page_ai > 0">
              <i class="el-icon-s-help"></i>
              <span slot="title">智能设置</span>
            </el-menu-item>
            <el-menu-item index="/setting/audio" @click="saveNavState('/setting/audio')" v-show="formData.capInfo.support_page_audio > 0">
              <i class="el-icon-caret-right"></i>
              <span slot="title">音频设置</span>
            </el-menu-item>
            <el-menu-item index="/setting/video" @click="saveNavState('/setting/video')" v-show="formData.capInfo.support_page_video > 0">
              <i class="el-icon-caret-right"></i>
              <span slot="title">视频设置</span>
            </el-menu-item>
            <el-menu-item index="/setting/overlay" @click="saveNavState('/setting/overlay')" v-show="formData.capInfo.support_page_overlay > 0">
              <i class="el-icon-menu"></i>
              <span slot="title">图层叠加</span>
            </el-menu-item>
            <el-menu-item index="/setting/storage" @click="saveNavState('/setting/storage')" v-show="formData.capInfo.support_page_storage > 0">
              <i class="el-icon-film"></i>
              <span slot="title">存储设置</span>
            </el-menu-item>
            <el-menu-item index="/setting/playback" @click="saveNavState('/setting/playback')" v-show="formData.capInfo.support_page_playback > 0">
              <i class="el-icon-video-play"></i>
              <span slot="title">录像回放</span>
            </el-menu-item>
          </el-menu>
        </div>
      </el-aside>
      <el-main>
        <router-view :dual_mode="formData.capInfo.support_dual_sns" :img_dual_mode="formData.capInfo.img_page_support_dual_sns"></router-view>
      </el-main>
    </el-container>
  </div>
</template>

<script>
export default {
  data () {
    return {
      activePath: '/setting/system',
      formData: {
        capInfo: {
          support_dual_sns: 1,
          img_page_support_dual_sns: 1,
          support_page_sys: 1,
          support_page_cam: 0,
          support_page_img: 0,
          support_page_ai: 0,
          support_page_audio: 0,
          support_page_video: 0,
          support_page_overlay: 0,
          support_page_storage: 0,
          support_page_playback: 1,
        }
      }
    }
  },
  created () {
    this.getInfo()
    var path = window.sessionStorage.getItem('settingActivePath')
    if (path) {
      this.activePath = path
    }
    if (this.$router.path !== this.activePath) {
      this.$router.push(this.activePath)
    }
  },
  methods: {
    async getInfo () {
      try {
        const { data: res } = await this.$http.get('setting/capability')
        console.log('setting page, get return: ', res)
        if (res.meta.status === 200) {
          this.formData.capInfo.support_dual_sns = res.data.capInfo.support_dual_sns
          this.formData.capInfo.img_page_support_dual_sns = res.data.capInfo.img_page_support_dual_sns
          this.formData.capInfo.support_page_sys = res.data.capInfo.support_page_sys
          this.formData.capInfo.support_page_cam = res.data.capInfo.support_page_cam
          this.formData.capInfo.support_page_img = res.data.capInfo.support_page_img
          this.formData.capInfo.support_page_ai = res.data.capInfo.support_page_ai
          this.formData.capInfo.support_page_audio = res.data.capInfo.support_page_audio
          this.formData.capInfo.support_page_video = res.data.capInfo.support_page_video
          this.formData.capInfo.support_page_overlay = res.data.capInfo.support_page_overlay
          this.formData.capInfo.support_page_storage = res.data.capInfo.support_page_storage
          this.formData.capInfo.support_page_playback = res.data.capInfo.support_page_playback
        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    },
    saveNavState (activePath) {
      window.sessionStorage.setItem('settingActivePath', activePath)
    }
  }
}
</script>

<style lang="less" scoped>
.setting {
  height: 100%!important;
}
.seting-container {
  height: 100%!important;
}

.el-aside {
  background-color: #4c4c4c!important;

  .left-container {
    // height: 800px;
    background-color: #4c4c4c!important;
    .el-menu {
      height: 100%!important;
      border: none;
    }
  }
}
.el-main {
  height: 100%;
  background-color: #eaedf1;
}
</style>
