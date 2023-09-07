import Vue from 'vue'
import App from './App.vue'
import router from './router'
import axios from 'axios'
import videojs from 'video.js'
import './plugins/element.js'
import './assets/css/global.css'
import 'video.js/dist/video-js.css'
import Agent from 'agentkeepalive';

const keepaliveAgent = new Agent({
  maxSockets: 100,
  maxFreeSockets: 10,
  timeout: 60000,
  freeSocketKeepAliveTimeout: 30000
})

Vue.prototype.$video = videojs

// axios.defaults.baseURL = 'http://192.168.127.246:8080/action'

axios.interceptors.request.use(config => {
  // console.log(config)
  config.headers.Authorization = window.sessionStorage.getItem('token')
  return config
})

axios.interceptors.response.use(config => {
  return config
})

axios.defaults.httpAgent = keepaliveAgent

Vue.prototype.$http = axios

Vue.config.productionTip = false

new Vue({
  router,
  render: h => h(App)
}).$mount('#app')
