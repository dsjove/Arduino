// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#define CONFIG_LED_ILLUMINATOR_ENABLED 1
#define CONFIG_STREAMING_ENABLED 0

#include <esp_http_server.h>
#include <esp_timer.h>
#include <img_converters.h>
#include <fb_gfx.h>
#include <sdkconfig.h>
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include <esp32-hal-log.h>
#include "Camera.h"
#endif
#if CONFIG_LED_ILLUMINATOR_ENABLED
#include "Flash.h"
#endif
#include "camera_index.h"

#define PART_BOUNDARY "123456789000000000000987654321"

#if CONFIG_STREAMING_ENABLED
static const char _STREAM_CONTENT_TYPE[] = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char _STREAM_BOUNDARY[] = "\r\n--" PART_BOUNDARY "\r\n";
static const char _STREAM_PART[] = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";
httpd_handle_t stream_httpd = NULL;
#endif

httpd_handle_t camera_httpd = NULL;

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
typedef struct {
  size_t size;   //number of values used for filtering
  size_t index;  //current value index
  size_t count;  //value count
  int sum;
  int *values;  //array to be filled with values
} ra_filter_t;

static ra_filter_t ra_filter;

static ra_filter_t *ra_filter_init(ra_filter_t *filter, size_t sample_size) {
  memset(filter, 0, sizeof(ra_filter_t));

  filter->values = (int *)malloc(sample_size * sizeof(int));
  if (!filter->values) {
    return NULL;
  }
  memset(filter->values, 0, sample_size * sizeof(int));

  filter->size = sample_size;
  return filter;
}

static int ra_filter_run(ra_filter_t *filter, int value) {
  if (!filter->values) {
    return value;
  }
  filter->sum -= filter->values[filter->index];
  filter->values[filter->index] = value;
  filter->sum += filter->values[filter->index];
  filter->index++;
  filter->index = filter->index % filter->size;
  if (filter->count < filter->size) {
    filter->count++;
  }
  return filter->sum / filter->count;
}
#endif

static esp_err_t bmp_handler(httpd_req_t *req) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  uint64_t fr_start = esp_timer_get_time();
#endif
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
int buf_len;
#endif
esp_err_t res;
{
  Camera::Frame frame(CONFIG_LED_ILLUMINATOR_ENABLED);
  if (!frame) {
    log_e("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  Camera::BMP bmp(frame);
  if (!bmp) {
    log_e("BMP Conversion failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }
  
  httpd_resp_set_type(req, "image/x-windows-bmp");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.bmp");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

  char ts[32];
  snprintf(ts, 32, "%lld.%06ld", frame.timestamp.tv_sec, frame.timestamp.tv_usec);
  httpd_resp_set_hdr(req, "X-Timestamp", (const char *)ts);

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  buf_len = bmp.length();
#endif
  res = httpd_resp_send(req, (const char *)bmp.buffer(), bmp.length());
}
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  uint64_t fr_end = esp_timer_get_time();
#endif
  log_i("BMP: %llums, %uB", (uint64_t)((fr_end - fr_start) / 1000), buf_len);
  return res;
}

struct jpg_chunking_t {
  bool streamed;
  //FILE* file; FILE *file = fopen("/sdcard/image.jpg", "wb");
  timeval timestamp;
  httpd_req_t *req;
  size_t len;
  //    //fclose(file);
};

static size_t jpg_encode_stream(void *arg, size_t index, const void *data, size_t len) {
  /*
  if (file) {
        fwrite(data, len, 1, file);  // Append chunk to file
    }
  */
  jpg_chunking_t *j = (jpg_chunking_t *)arg;
  if (index == Camera::Frame::SingleIndex) {
    if (httpd_resp_send(j->req, (const char *)data, len) == ESP_OK) {
      j->len = len;
      return len;
    }
    return 0;
  }
  if (index == 0) {
    j->len = 0;
  }
  if (httpd_resp_send_chunk(j->req, (const char *)data, len) != ESP_OK) {
    return 0;
  }
  j->len += len;
  return len;
}

static esp_err_t capture_handler(httpd_req_t *req) {
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  int64_t fr_start = esp_timer_get_time();
#endif
  esp_err_t res = ESP_OK;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  size_t buf_len = 0;
#endif
{
  Camera::Frame frame(CONFIG_LED_ILLUMINATOR_ENABLED);

  if (!frame) {
    log_e("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  char ts[32];
  snprintf(ts, 32, "%lld.%06ld", frame.timestamp.tv_sec, frame.timestamp.tv_usec);
  httpd_resp_set_hdr(req, "X-Timestamp", (const char *)ts);

  jpg_chunking_t jchunk = {false, frame.timestamp, req, 0};
  res = frame.jpg(jpg_encode_stream, &jchunk, false, 80) ? ESP_OK : ESP_FAIL;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  buf_len = jchunk.len;
#endif
}
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  int64_t fr_end = esp_timer_get_time();
#endif
  log_i("JPG: %uB %ums", (uint32_t)(buf_len), (uint32_t)((fr_end - fr_start) / 1000));
  return res;
}

#if CONFIG_STREAMING_ENABLED
static esp_err_t stream_handler(httpd_req_t *req) {
  static int64_t last_frame = 0;
  if (!last_frame) {
    last_frame = esp_timer_get_time();
  }

  esp_err_t res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "X-Framerate", "60");

  Flash::On on(CONFIG_LED_ILLUMINATOR_ENABLED);

  while (true) {
    size_t _jpg_buf_len = 0;
    {
      bool converted = false;
      uint8_t *_jpg_buf = NULL;
      Camera::Frame frame(false);
      if (!frame) {
        log_e("Camera capture failed");
        res = ESP_FAIL;
      } else {
        if (frame.fb->format != PIXFORMAT_JPEG) {
          converted = frame2jpg(frame.fb, 80, &_jpg_buf, &_jpg_buf_len);
          frame.consume();
          if (!converted) {
            log_e("JPEG compression failed");
            res = ESP_FAIL;
          }
        } else {
          _jpg_buf_len = frame.fb->len;
          _jpg_buf = frame.fb->buf;
        }
      }
      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, sizeof(_STREAM_BOUNDARY)-1);
      }
      if (res == ESP_OK) {
        char part_buf[128];
        size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, _jpg_buf_len, (int)frame.timestamp.tv_sec, (int)frame.timestamp.tv_usec);
        res = httpd_resp_send_chunk(req, part_buf, hlen);
      }
      if (res == ESP_OK) {
        res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
      }
      if (converted && _jpg_buf) {
        free(_jpg_buf);
      }
      if (res != ESP_OK) {
        log_e("Send frame failed");
        break;
      }
    }
    int64_t fr_end = esp_timer_get_time();
    int64_t frame_time = fr_end - last_frame;
    last_frame = fr_end;

    frame_time /= 1000;
#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
    uint32_t avg_frame_time = ra_filter_run(&ra_filter, frame_time);
#endif
    log_i(
      "MJPG: %uB %ums (%.1ffps), AVG: %ums (%.1ffps)", (uint32_t)(_jpg_buf_len), (uint32_t)frame_time, 1000.0 / (uint32_t)frame_time, avg_frame_time,
      1000.0 / avg_frame_time
    );
  }

  return res;
}
#endif

static esp_err_t extract_query_str(httpd_req_t *req, char **obuf) {
  size_t buf_len = httpd_req_get_url_query_len(req) + 1;
  if (buf_len > 1) {
    char *buf = (char *)malloc(buf_len);
    if (!buf) {
      httpd_resp_send_500(req);
      return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
      *obuf = buf;
      return ESP_OK;
    }
    free(buf);
  }
  httpd_resp_send_404(req);
  return ESP_FAIL;
}

static esp_err_t cmd_handler(httpd_req_t *req) {
  char *qry = NULL;
  if (extract_query_str(req, &qry) != ESP_OK) {
    return ESP_FAIL;
  }

  char variable[32];
  char value[32];
  if (httpd_query_key_value(qry, "var", variable, sizeof(variable)) != ESP_OK || httpd_query_key_value(qry, "val", value, sizeof(value)) != ESP_OK) {
    free(qry);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(qry);

  int val = atoi(value);
  log_i("%s = %d", variable, val);
  bool res = Camera::setValue(variable, val);

  if (!res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t status_handler(httpd_req_t *req) {
  static char json_response[1024];
  Camera::getValues(json_response);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, json_response, strlen(json_response));
}

static esp_err_t xclk_handler(httpd_req_t *req) {
  char *qry = NULL;
  if (extract_query_str(req, &qry) != ESP_OK) {
    return ESP_FAIL;
  }

  char _xclk[32];
  if (httpd_query_key_value(qry, "xclk", _xclk, sizeof(_xclk)) != ESP_OK) {
    free(qry);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(qry);

  int xclk = atoi(_xclk);
  log_i("Set XCLK: %d MHz", xclk);

  int res = Camera::setXLCK(xclk);
  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t reg_handler(httpd_req_t *req) {
  char *qry = NULL;
  if (extract_query_str(req, &qry) != ESP_OK) {
    return ESP_FAIL;
  }

  char _reg[32];
  char _mask[32];
  char _val[32];
  if (httpd_query_key_value(qry, "reg", _reg, sizeof(_reg)) != ESP_OK || httpd_query_key_value(qry, "mask", _mask, sizeof(_mask)) != ESP_OK
      || httpd_query_key_value(qry, "val", _val, sizeof(_val)) != ESP_OK) {
    free(qry);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(qry);

  int reg = atoi(_reg);
  int mask = atoi(_mask);
  int val = atoi(_val);
  log_i("Set Register: reg: 0x%02x, mask: 0x%02x, value: 0x%02x", reg, mask, val);

  int res = Camera::setReg(reg, mask, val);
  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t greg_handler(httpd_req_t *req) {
  char *qry = NULL;
  if (extract_query_str(req, &qry) != ESP_OK) {
    return ESP_FAIL;
  }

  char _reg[32];
  char _mask[32];
  if (httpd_query_key_value(qry, "reg", _reg, sizeof(_reg)) != ESP_OK || httpd_query_key_value(qry, "mask", _mask, sizeof(_mask)) != ESP_OK) {
    free(qry);
    httpd_resp_send_404(req);
    return ESP_FAIL;
  }
  free(qry);

  int reg = atoi(_reg);
  int mask = atoi(_mask);
  int res = Camera::getReg(reg, mask);
  if (res < 0) {
    return httpd_resp_send_500(req);
  }
  log_i("Get Register: reg: 0x%02x, mask: 0x%02x, value: 0x%02x", reg, mask, res);

  char buffer[20];
  const char *val = itoa(res, buffer, 10);
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, val, strlen(val));
}

static int httpd_query_key_int(const char *qry, const char *key, int def) {
  char _int[32];
  if (httpd_query_key_value(qry, key, _int, sizeof(_int)) != ESP_OK) {
    return def;
  }
  return atoi(_int);
}

static esp_err_t pll_handler(httpd_req_t *req) {
  char *qry = NULL;
  if (extract_query_str(req, &qry) != ESP_OK) {
    return ESP_FAIL;
  }

  Camera::PLL pll;
  pll.bypass = httpd_query_key_int(qry, "bypass", 0);
  pll.mul = httpd_query_key_int(qry, "mul", 0);
  pll.sys = httpd_query_key_int(qry, "sys", 0);
  pll.root = httpd_query_key_int(qry, "root", 0);
  pll.pre = httpd_query_key_int(qry, "pre", 0);
  pll.seld5 = httpd_query_key_int(qry, "seld5", 0);
  pll.pclken = httpd_query_key_int(qry, "pclken", 0);
  pll.pclk = httpd_query_key_int(qry, "pclk", 0);
  free(qry);

  log_i("Set Pll: bypass: %d, mul: %d, sys: %d, root: %d, pre: %d, seld5: %d, pclken: %d, pclk: %d", pll.bypass, pll.mul, pll.sys, pll.root, pll.pre, pll.seld5, pll.pclken, pll.pclk);
  int res = Camera::setPLL(pll);
  if (res) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t win_handler(httpd_req_t *req) {
  char *qry = NULL;
  if (extract_query_str(req, &qry) != ESP_OK) {
    return ESP_FAIL;
  }

  Camera::Resolution res;
  res.startX = httpd_query_key_int(qry, "sx", 0);
  res.startY = httpd_query_key_int(qry, "sy", 0);
  res.endX = httpd_query_key_int(qry, "ex", 0);
  res.endY = httpd_query_key_int(qry, "ey", 0);
  res.offsetX = httpd_query_key_int(qry, "offx", 0);
  res.offsetY = httpd_query_key_int(qry, "offy", 0);
  res.totalX = httpd_query_key_int(qry, "tx", 0);
  res.totalY = httpd_query_key_int(qry, "ty", 0);  // codespell:ignore totaly
  res.outputX = httpd_query_key_int(qry, "ox", 0);
  res.outputY = httpd_query_key_int(qry, "oy", 0);
  res.scale = httpd_query_key_int(qry, "scale", 0) == 1;
  res.binning = httpd_query_key_int(qry, "binning", 0) == 1;
  free(qry);

  log_i(
    "Set Window: Start: %d %d, End: %d %d, Offset: %d %d, Total: %d %d, Output: %d %d, Scale: %u, Binning: %u", res.startX, res.startY, res.endX, res.endY, res.offsetX, res.offsetY,
    res.totalX, res.totalY, res.outputX, res.outputY, res.scale, res.binning  // codespell:ignore totaly
  );
  int result = Camera::setResolution(res); // codespell:ignore totaly
  if (result) {
    return httpd_resp_send_500(req);
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  return httpd_resp_send(req, NULL, 0);
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  uint16_t PID = Camera::getPID();
  if (PID == OV3660_PID) {
    return httpd_resp_send(req, (const char *)index_ov3660_html_gz, index_ov3660_html_gz_len);
  } else if (PID == OV5640_PID) {
    return httpd_resp_send(req, (const char *)index_ov5640_html_gz, index_ov5640_html_gz_len);
  } else {
    return httpd_resp_send(req, (const char *)index_ov2640_html_gz, index_ov2640_html_gz_len);
  }
}

bool startCameraServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.max_uri_handlers = 16;

  httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t status_uri = {
    .uri = "/status",
    .method = HTTP_GET,
    .handler = status_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t cmd_uri = {
    .uri = "/control",
    .method = HTTP_GET,
    .handler = cmd_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

#if CONFIG_STREAMING_ENABLED
  httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };
#endif
  httpd_uri_t bmp_uri = {
    .uri = "/bmp",
    .method = HTTP_GET,
    .handler = bmp_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t xclk_uri = {
    .uri = "/xclk",
    .method = HTTP_GET,
    .handler = xclk_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t reg_uri = {
    .uri = "/reg",
    .method = HTTP_GET,
    .handler = reg_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t greg_uri = {
    .uri = "/greg",
    .method = HTTP_GET,
    .handler = greg_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t pll_uri = {
    .uri = "/pll",
    .method = HTTP_GET,
    .handler = pll_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

  httpd_uri_t win_uri = {
    .uri = "/resolution",
    .method = HTTP_GET,
    .handler = win_handler,
    .user_ctx = NULL
#ifdef CONFIG_HTTPD_WS_SUPPORT
    ,
    .is_websocket = true,
    .handle_ws_control_frames = false,
    .supported_subprotocol = NULL
#endif
  };

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_INFO
  ra_filter_init(&ra_filter, 20);
#endif

  log_i("Starting web server on port: '%d'", config.server_port);
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    // Main page
    httpd_register_uri_handler(camera_httpd, &index_uri);

    // Basic Settings
    httpd_register_uri_handler(camera_httpd, &cmd_uri);
    httpd_register_uri_handler(camera_httpd, &status_uri);

    // Images
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    httpd_register_uri_handler(camera_httpd, &bmp_uri);

    // Advanced Register
    httpd_register_uri_handler(camera_httpd, &reg_uri);
    httpd_register_uri_handler(camera_httpd, &greg_uri);

    // Advanced Clock
    httpd_register_uri_handler(camera_httpd, &xclk_uri);

    // Advanced PLL?
    httpd_register_uri_handler(camera_httpd, &pll_uri);

    // Advanced Resolution
    httpd_register_uri_handler(camera_httpd, &win_uri);
#if !CONFIG_STREAMING_ENABLED
    return true;
#endif
  }
#if CONFIG_STREAMING_ENABLED
  config.server_port += 1;
  config.ctrl_port += 1;
  log_i("Starting stream server on port: '%d'", config.server_port);
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    return true;
  }
#endif
  return false;
}
