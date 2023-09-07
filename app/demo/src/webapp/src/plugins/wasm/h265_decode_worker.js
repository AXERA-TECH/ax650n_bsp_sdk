importScripts("./libffmpeg_265.js")

var packets = []
var tick = 0
var loaded = false
Module.onRuntimeInitialized = () => {
  loaded = true
  openDecoder()
}
var pts_v = 0;
function openDecoder() {
  var videoCallback = Module.addFunction(function (addr_data, size, width, height, pts) {
    let u8s = Module.HEAPU8.subarray(addr_data, addr_data + size)
    let data  = new Uint8Array(u8s);
    var obj = {
        type:'yuv',
        data: data,
        width,
        height
    }
    if (pts == 100) {
      var delay = performance.now() - tick;
      console.log("=======================", delay) ;
    }
    postMessage(obj);
}, "viiiii");

  var LOG_LEVEL_WASM = 1;
  var DECODER_H265 = 1;
  var decoder_type = DECODER_H265;
  var ret = Module._openDecoder(decoder_type, videoCallback, LOG_LEVEL_WASM)
  if(ret == 0) {
      console.log("openDecoder success");
  } else {
      console.error("openDecoder failed with error", ret);
      return;
  }
}

onmessage = function(event) {
  var worker_data = event.data;
  var type = worker_data.type;
  switch(type) {
    case "h265":
      var data = new Uint8Array(worker_data.data)
    if (loaded) {
        if (pts_v == 100) {
          tick = performance.now();
        }
        let fileSize = data.byteLength
        let cacheBuffer = Module._malloc(fileSize)
        Module.HEAPU8.set(data, cacheBuffer)
        Module._decodeData(cacheBuffer, fileSize, pts_v++)
        Module._free(cacheBuffer)
      }
    case "load_wasm":
    default:
      break;
  }
}