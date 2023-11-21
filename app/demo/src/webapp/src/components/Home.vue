<template>
  <div class="home">
    <el-container class="home-container">
      <el-header>
        <div class="header_logo">
          <img alt="logo" src="../assets/logo.png">
        </div>
        <div class="header_ver">
          <span>{{appName}} SDK: {{sdkVersion}}</span>
        </div>
        <div>
          <el-divider direction="vertical"></el-divider>
        </div>
        <div class="header_nav">
          <el-menu router :default-active="activePath" @select="handleSelect" mode="horizontal" background-color="#4c4c4c" text-color="#fff" active-text-color="#fff">
            <el-menu-item index="/preview" @click="saveNavState('/preview')" style="height: 50px;line-height: 50px;">预览</el-menu-item>
            <el-menu-item index="/setting" @click="saveNavState('/setting')" style="height: 50px;line-height: 50px;">配置</el-menu-item>
          </el-menu>
        </div>
        <div class="header_right" align="right">
            <el-select class="mode-select" v-model="mode_option_data" size="mini" style="width:150px">
              <el-option v-for="item in mode_options" :key="item.value" :label="item.label" :value="item.value"> </el-option>
            </el-select>
            <i class="el-icon-s-custom"/>
            <span>admin</span>
            <el-tooltip effect="dark" content="退出登录" placement="bottom-end" :enterable="false">
              <el-button type="text" @click="logout" icon="el-icon-switch-button"></el-button>
            </el-tooltip>
        </div>
      </el-header>
      <el-main class="main-container">
        <router-view :mode_option="mode_option_data" :home_dual_mode="sns_mode" :pano_sns_id="pano_snsId"></router-view>
      </el-main>
    </el-container>
  </div>
</template>

<script>
export default {
  data () {
    return {
      appName: '--',
      appVersion: '--',
      sdkVersion: '--',
      activePath: '/preview',
      mode_options: [
        {
          label: '普通模式',
          value: 'normal'
        },
        {
          label: '智能模式',
          value: 'ai'
        }
      ],
      mode_option_data: 'normal',
      sns_mode: parseInt(window.sessionStorage.getItem('snsMode')),
      pano_snsId: parseInt(window.sessionStorage.getItem('panoSnsId')),
      isForceStop: false
    }
  },
  created () {
    var reloadFlag = window.sessionStorage.getItem('reloadFlag')
    if (reloadFlag == 'true') {
        window.sessionStorage.setItem('reloadFlag', 'false')
        window.location.reload(true)
        return
    }
    console.log('home created ++')
    // this.$router.push('/preview')
    var baseUrl = window.location.href
    baseUrl = baseUrl.substring(0, baseUrl.indexOf('8080/') + 5) + 'action'
    this.$http.defaults.baseURL = baseUrl
    this.getInfo()

    var path = window.sessionStorage.getItem('homeActivePath')
    if (path) {
      this.activePath = path
    }
    console.log('home:', this.activePath)
    if (this.$router.path !== this.activePath) {
      this.$router.push(this.activePath)
    }

    this.isForceStop = false
    this.startRecvEvents()

    console.log('home created --')
  },
  methods: {
    async getInfo () {
      try {
        const { data: res } = await this.$http.get('setting/system')
        console.log('home page, system get return: ', res)
        if (res.meta.status === 200) {
          this.appName = res.data.appName
          this.appVersion = res.data.appVersion
          this.sdkVersion = res.data.sdkVersion
        }
      } catch (error) {
        this.$message.error('获取信息失败')
      }
    },
    handleSelect (key, keyPath) {
      console.log(key, keyPath)
    },
    logout () {
      window.sessionStorage.clear()
      this.isForceStop = true
      this.stopRecvEvents()
      this.$router.push('/login')
    },
    saveNavState (activePath) {
      window.sessionStorage.setItem('homeActivePath', activePath)
    },
    startRecvEvents () {
      console.log('[home] startRecvEvents ++')
      this.stopRecvEvents()
      const tokenStr = window.sessionStorage.getItem('token')
      var uri = 'ws://' + window.location.host + '/ws/events?token=' + tokenStr
      var protocol = 'binary'
      this.wsEvents = new WebSocket(uri, protocol)
      this.wsEvents.onopen = () => {
        console.log('[home] ws events connected')
        this.createEventstimer()
      }
      this.wsEvents.onclose = (err) => {
        console.log('[home] ws events disconnected', err)
        if (!this.isForceStop) {
          this.startRecvEvents()
        }
      }
      this.wsEvents.onmessage = (event) => {
        if (typeof (event.data) === 'string') {
          console.log('[home] got a ws events message: ' + event.data)
        } else {
          try {
            var reader = new FileReader()
            reader.onload = (evt) => {
              if (evt.target.readyState === FileReader.DONE) {
                console.log('[home] got a event: ' + evt.target.result)
                var eventObj = JSON.parse(evt.target.result)
                for (var i = 0;i < eventObj.events.length;i++) {
                  if (eventObj.events[i].type === 0) {
                    this.getInfo()
                  } else if (eventObj.events[i].type === 4) {
                    this.logout()
                  }
                }
              }
            }
            reader.readAsText(event.data)
          } catch (err) {
            console.log('[home] ws events catch except: ' + err)
          }
        }
      }
      this.wsEvents.onerror = (err) => {
        console.log('[home] ws events occure error', err)
      }
      console.log('[home] startRecvEvents --')
    },

    stopRecvEvents() {
      console.log('[home] stopRecvEvents ++')
      this.clearEventstimer()
      if (this.wsEvents) {
        this.wsEvents.close()
      }
      console.log('[home] stopRecvEvents --')
    },
    createEventstimer() {
      this.clearEventstimer()
      this.timerEvents = setInterval(() => {
        if (this.wsEvents) {
          this.wsEvents.send("keep-alive")
        }
      }, 3000)
    },
    clearEventstimer() {
      if (this.timerEvents) {
        clearInterval(this.timerEvents)
        this.timerEvents = undefined
      }
    }
  }
}
</script>

<style lang="less" scoped>
.home {
  height: 100%;
  background-color: #333333;
}
.home-container {
  height: 100%;
}
.el-header {
  height: 50px!important;
  width: 100%;
  background-color: #4c4c4c;
  display: flex;
  justify-content: space-between;
  padding-left: 0;
  align-items: center;
  color: #fff;
  .header_logo {
    img {
      height: 50px;
    }
  }
  .header_ver {
    span {
      white-space: nowrap;
      display: inline-block;
      text-overflow: ellipsis;
      line-height: 0.9;
    }
  }
  .header_nav {
    width: 100%;
    font-weight: bold;
    font-size: 20px;
    .el-menu {
      align-items: left;
      border-bottom: none;
    }
  }
  .header_right {
    float: right;
    vertical-align: middle;
    width: 100%;
    box-sizing: border-box;
    // display: block;
    white-space:nowrap;
    > span {
      font-size: 16px;
      font-family: 'Arial';
      margin-left: 4px;
      display: inline-block;
    }
    .el-button {
      margin-left: 20px;
      font-size: 20px;
      display: inline-block;
      color: #fff;
    }
    > i {
      font-size: 20px;
      display: inline-block;
      margin-left: 40px!important;
    }
  }
  .el-menu-item.is-active {
    border-bottom-color: #00FF00 !important;
  }
}
.el-main {
  height: 100%;
  padding-top: 10px!important;
  padding-left: 10px!important;
  padding-right: 10px!important;
}
.el-divider {
  height: 30px;
  width: 1px;
  margin-right: 30px;
  background-color: #777;
}
.mode-select {
  /deep/ .el-input__inner {
    height: 24px;
    border: none !important;
  }
}
</style>
