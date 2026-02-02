#pragma once

#include <Arduino.h>
#include <TaskScheduler.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <driver/i2s.h>

#include "src/PinIO/PinIO.h"

namespace mic
{
  // ---- Sense built-in PDM Microphone ----
  // I2S clock (BCLK / PDM CLK)
  inline constexpr PinIO<42, GpioMode::Delegated> MicBclk{};
  // I2S word select (LRCLK / PDM WS)
  inline constexpr PinIO<41, GpioMode::Delegated> MicLrc{};
  // I2S data in (PDM DATA)
  inline constexpr PinIO<2,  GpioMode::Delegated> MicDin{};

  namespace device
  {
    inline constexpr i2s_port_t port = I2S_NUM_0; // keep separate from your MAX98357A TX on I2S_NUM_0
  }

  struct Config
  {
    static constexpr uint32_t SampleRateHz = 16000; // Seeed notes 16kHz is stable for ESP32-S3 PDM
    static constexpr int      DmaBufCount  = 8;
    static constexpr int      DmaBufLen    = 256;
  };

  inline void begin(Scheduler&)
  {
    // PDM mic pins per XIAO ESP32S3 Sense:
    // DATA = GPIO41, CLK = GPIO42
    // (Seeed documentation) :contentReference[oaicite:2]{index=2}

    const i2s_config_t cfg = {
      .mode                 = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
      .sample_rate          = Config::SampleRateHz,
      .bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S),
      .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count        = Config::DmaBufCount,
      .dma_buf_len          = Config::DmaBufLen,
      .use_apll             = false,
      .tx_desc_auto_clear   = false,
      .fixed_mclk           = 0,

      // --- Newer legacy-struct fields (initialize to sane defaults) ---
      .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
      .bits_per_chan        = I2S_BITS_PER_CHAN_16BIT,

    #if SOC_I2S_SUPPORTS_TDM
      .chan_mask            = static_cast<i2s_channel_t>(I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1),
      .total_chan           = 2,
      .left_align           = false,
      .big_edin             = false,
      .bit_order_msb        = true,
      .skip_msk             = false,
    #endif
    };

    // Install (OK if already installed)
    esp_err_t err = i2s_driver_install(device::port, &cfg, 0, nullptr);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
      Serial.printf("[mic] i2s_driver_install failed: %d\n", static_cast<int>(err));
      return;
    }

    // For PDM RX: ESP-IDF uses "ws" as CLK and "data_in" as DATA
    i2s_pin_config_t pins_cfg {};
    pins_cfg.bck_io_num   = MicBclk.pin;
    pins_cfg.ws_io_num    = MicLrc.pin;
    pins_cfg.data_out_num = I2S_PIN_NO_CHANGE;
    pins_cfg.data_in_num  = MicDin.pin;
    pins_cfg.mck_io_num   = I2S_PIN_NO_CHANGE;

    err = i2s_set_pin(device::port, &pins_cfg);
    if (err != ESP_OK)
    {
      Serial.printf("[mic] i2s_set_pin failed: %d\n", static_cast<int>(err));
      return;
    }

    i2s_zero_dma_buffer(device::port);
  }
}
