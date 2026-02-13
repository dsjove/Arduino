#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <cstring>   // memcmp, memset

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_err.h" // ESP_ERROR_CHECK

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// NOTE:
// - This file is written to compile under typical Arduino-ESP32 C++17 settings.
// - If you're using the XIAO ESP32S3 Sense *built-in* SD slot, you may need SD_MMC instead of SD.
//   (This file keeps SD.h as you had it.)

class SbjAudioPlayerEsp32S3 {
public:
  struct Config {
    // I2S pins to MAX98357A (your proven wiring)
    gpio_num_t pinDout;
    gpio_num_t pinBclk;
    gpio_num_t pinLrc;

    // Behavior
    uint32_t loopDelayMs    = 5000; // gap between queued plays
    uint32_t flushSilenceMs = 200;  // prevents “last few ms repeating”
    size_t   queueDepth     = 4;    // request queue depth
  };

  explicit SbjAudioPlayerEsp32S3(int pinDout, int pinBclk, int pinLrc)
  : cfg_({
	arduinoPinToGpio(pinDout),
	arduinoPinToGpio(pinBclk),
	arduinoPinToGpio(pinLrc)}) {}

  bool begin() {
    q_ = xQueueCreate((UBaseType_t)cfg_.queueDepth, sizeof(Req));
    if (q_ != nullptr)
    {
      return startTask();
    }
    return false;
  }

  // Queue request (thread-safe from other tasks)
  bool playOnce(const char* path) {
  Serial.println("play");
    if (!q_) return false;
    if (!path || !*path) return false;

    Req r{};
    copyPath_(r.path, sizeof(r.path), path);
    if (!r.path[0]) return false;

    // enqueue (don’t block)
    auto d = xQueueSend(q_, &r, 0) == pdTRUE;
	return d;
  }

  bool isPlaying() const { return playing_; }

  void debugPrint() {
    Serial.printf("Audio D-%d Bclk-%d Lrc-%d\n",
      gpioToArduinoPin(cfg_.pinDout), gpioToArduinoPin(cfg_.pinBclk), gpioToArduinoPin(cfg_.pinLrc) );
  }

  uint32_t loopDelayMs() const { return cfg_.loopDelayMs; }
  void setLoopDelayMs(uint32_t ms) { cfg_.loopDelayMs = ms; }

  // Start background task that runs the player loop.
  // Call once after begin().
  bool startTask(const char* name = "Audio",
                 uint32_t stackWords = 8192,
                 UBaseType_t prio = 2,
                 BaseType_t core = tskNO_AFFINITY) {
    if (task_) return true;

    auto thunk = [](void* self) {
      static_cast<SbjAudioPlayerEsp32S3*>(self)->taskLoop_();
    };

    BaseType_t ok = xTaskCreatePinnedToCore(thunk, name, stackWords, this, prio, &task_, core);
    return ok == pdPASS;
  }

private:
  struct Req { char path[96]; };

  struct WavInfo {
    uint16_t fmt = 0;       // 1 = PCM
    uint16_t ch  = 0;       // 1 or 2
    uint32_t sr  = 0;       // sample rate
    uint16_t bits= 0;       // 16
    uint32_t dataOff = 0;
    uint32_t dataSize= 0;
  };

  Config cfg_;
  QueueHandle_t q_ = nullptr;
  TaskHandle_t  task_ = nullptr;

  // I2S
  i2s_chan_handle_t tx_ = nullptr;

  volatile bool playing_ = false;

  static void copyPath_(char* dst, size_t cap, const char* src) {
    if (!dst || cap == 0) return;
    size_t i = 0;
    for (; i + 1 < cap && src && src[i]; ++i) dst[i] = src[i];
    dst[i] = '\0';
  }

  static uint16_t rd16_(File& f) {
    uint8_t b[2];
    if (f.read(b, 2) != 2) return 0;
    return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
  }

  static uint32_t rd32_(File& f) {
    uint8_t b[4];
    if (f.read(b, 4) != 4) return 0;
    return (uint32_t)b[0]
        | ((uint32_t)b[1] << 8)
        | ((uint32_t)b[2] << 16)
        | ((uint32_t)b[3] << 24);
  }

  static bool parseWav_(File& f, WavInfo& w) {
    f.seek(0);

    char riff[4] = {};
    char wave[4] = {};
    if (f.readBytes(riff, 4) != 4) return false;
    (void)rd32_(f); // file size
    if (f.readBytes(wave, 4) != 4) return false;

    if (memcmp(riff, "RIFF", 4) != 0 || memcmp(wave, "WAVE", 4) != 0) return false;

    bool gotFmt = false, gotData = false;

    while (f.available() && !(gotFmt && gotData)) {
      char id[4] = {};
      if (f.readBytes(id, 4) != 4) break;
      uint32_t sz = rd32_(f);

      // RIFF chunks are padded to even sizes
      auto padded = [](uint32_t n) -> uint32_t { return n + (n & 1U); };

      if (memcmp(id, "fmt ", 4) == 0) {
        w.fmt  = rd16_(f);
        w.ch   = rd16_(f);
        w.sr   = rd32_(f);
        (void)rd32_(f); // byte rate
        (void)rd16_(f); // block align
        w.bits = rd16_(f);

        if (sz > 16) {
          uint32_t extra = padded(sz - 16);
          f.seek(f.position() + extra);
        } else if (sz < 16) {
          // malformed fmt
          return false;
        }

        gotFmt = true;
      } else if (memcmp(id, "data", 4) == 0) {
        w.dataOff  = f.position();
        w.dataSize = sz;
        gotData = true;

        // Do not seek past data; caller will seek to dataOff.
      } else {
        f.seek(f.position() + padded(sz));
      }
    }

    return gotFmt && gotData;
  }

  void i2sStart_(uint32_t sampleRate) {
    if (tx_) {
      i2s_channel_disable(tx_);
      i2s_del_channel(tx_);
      tx_ = nullptr;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_, nullptr));

    // C++17-safe init (no designated initializers)
    i2s_std_config_t std_cfg = {};
    std_cfg.clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sampleRate);
    //std_cfg.slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO);
    std_cfg.slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
  I2S_DATA_BIT_WIDTH_16BIT,
  I2S_SLOT_MODE_STEREO
);

    std_cfg.gpio_cfg.mclk = I2S_GPIO_UNUSED;
    std_cfg.gpio_cfg.bclk = cfg_.pinBclk;
    std_cfg.gpio_cfg.ws   = cfg_.pinLrc;
    std_cfg.gpio_cfg.dout = cfg_.pinDout;
    std_cfg.gpio_cfg.din  = I2S_GPIO_UNUSED;

    std_cfg.gpio_cfg.invert_flags.mclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.bclk_inv = false;
    std_cfg.gpio_cfg.invert_flags.ws_inv   = false;

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_));
  }

  void writeSilenceMs_(uint32_t ms, uint32_t sr) {
    const uint32_t frames = (sr * ms) / 1000;

    static int16_t zeros[512 * 2];
    memset(zeros, 0, sizeof(zeros));

    uint32_t left = frames;
    while (left) {
      uint32_t chunkFrames = (left > 512) ? 512 : left; // frames (stereo frame = 4 bytes at 16-bit)
      size_t bw = 0;
      ESP_ERROR_CHECK(i2s_channel_write(tx_, zeros, chunkFrames * 4, &bw, portMAX_DELAY));
      left -= chunkFrames;
    }
  }

  void playFileOnce_(const char* path, ) {
    File f = SD.open(path, FILE_READ);
    if (!f) return;

    WavInfo w{};
    if (!parseWav_(f, w)) { f.close(); return; }
    if (w.fmt != 1 || w.bits != 16 || (w.ch != 1 && w.ch != 2)) { f.close(); return; }

    i2sStart_(w.sr);
    f.seek(w.dataOff);

    playing_ = true;

    static uint8_t raw[4096];
    uint32_t rem = w.dataSize;

    while (rem) {
      int n = f.read(raw, (rem < sizeof(raw)) ? rem : sizeof(raw));
      if (n <= 0) break;

      size_t bw = 0;

      if (w.ch == 2) {
        // Stereo PCM16 little-endian
        ESP_ERROR_CHECK(i2s_channel_write(tx_, raw, (size_t)n, &bw, portMAX_DELAY));
      } else {
        // Mono -> duplicate to stereo (no truncation)
        const int samples = n / 2; // 16-bit mono samples
        const int16_t* mono = (const int16_t*)raw;

        // raw is 4096 bytes max => samples <= 2048 => stereo samples <= 4096 int16
        static int16_t stereo[4096];
        for (int i = 0; i < samples; ++i) {
          stereo[i * 2]     = mono[i];
          stereo[i * 2 + 1] = mono[i];
        }
        ESP_ERROR_CHECK(i2s_channel_write(tx_, stereo, (size_t)samples * 4, &bw, portMAX_DELAY));
      }

      rem -= (uint32_t)n;
      vTaskDelay(0); // yield
    }

    // flush tail + stop clocking during gap
    writeSilenceMs_(cfg_.flushSilenceMs, w.sr);
    i2s_channel_disable(tx_);

    f.close();
    playing_ = false;
  }

  void taskLoop_() {
    Req r{};
    for (;;) {
      if (xQueueReceive(q_, &r, portMAX_DELAY) == pdTRUE) {
        playFileOnce_(r.path);
        vTaskDelay(pdMS_TO_TICKS(cfg_.loopDelayMs));
      }
    }
  }
};

// If this file is included by more than one translation unit, 'inline' prevents multiple-definition.
/*
inline SbjAudioPlayerEsp32S3 player;

void setup() {
  Serial.begin(115200);

  if (!player.begin()) {
    Serial.println("Audio player begin() failed (SD init or queue create).");
    return;
  }

  player.setLoopDelayMs(5000);

  if (!player.startTask()) {
    Serial.println("Audio task start failed.");
    return;
  }

  player.playOnce("/Pink-Panther.wav");
}

void loop() {
  // nothing required
}
*/
