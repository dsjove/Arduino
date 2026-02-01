#pragma once

#include <Arduino.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <driver/i2s.h>
#pragma GCC diagnostic pop

#include "src/PinIO/PinIO.h"

// Audio output: MAX98357A I2S DAC/amp
// Target: Seeed XIAO ESP32S3 Sense
//
// Setup-only: installs/configures ESP32 I2S TX and routes BCLK/LRCK/DIN pins.
// No playback/business logic included.

namespace MAX98357AAudio
{
  inline constexpr PinIO<D2, GpioMode::DigitalOut> I2sDin{};
  inline constexpr PinIO<D6, GpioMode::DigitalOut> I2sBclk{};
  inline constexpr PinIO<D7, GpioMode::DigitalOut> I2sLrc{};

  struct Config
  {
    static constexpr i2s_port_t Port = I2S_NUM_0;

    // Safe, common defaults for MAX98357A
    static constexpr uint32_t SampleRateHz = 44100;
    static constexpr i2s_bits_per_sample_t BitsPerSample = I2S_BITS_PER_SAMPLE_16BIT;

    // Most MAX98357A examples run stereo frames; amp will happily play either.
    static constexpr i2s_channel_fmt_t ChannelFormat = I2S_CHANNEL_FMT_RIGHT_LEFT;
    static constexpr i2s_comm_format_t CommFormat =
      static_cast<i2s_comm_format_t>(I2S_COMM_FORMAT_STAND_I2S);

    // DMA buffering (tune later if needed)
    static constexpr int DmaBufCount = 8;
    static constexpr int DmaBufLen   = 256;
  };

  // Setup-only begin. Safe to call more than once.
  inline void begin()
  {
    // Configure I2S for TX only
    const i2s_config_t cfg = {
      .mode                 = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate          = Config::SampleRateHz,
      .bits_per_sample      = Config::BitsPerSample,
      .channel_format       = Config::ChannelFormat,
      .communication_format = Config::CommFormat,
      .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count        = Config::DmaBufCount,
      .dma_buf_len          = Config::DmaBufLen,
      .use_apll             = false,
      .tx_desc_auto_clear   = true,
      .fixed_mclk           = 0,

      // --- Newer legacy-struct fields (initialize to sane defaults) ---
      .mclk_multiple        = I2S_MCLK_MULTIPLE_256,
      .bits_per_chan        = I2S_BITS_PER_CHAN_16BIT,

    #if SOC_I2S_SUPPORTS_TDM
      .chan_mask            = static_cast<i2s_channel_t>(I2S_TDM_ACTIVE_CH0 | I2S_TDM_ACTIVE_CH1),
      .total_chan           = 2,
      .left_align           = false,
      .big_edin             = false,
      .bit_order_msb        = true,   // typical for I2S; adjust if your DAC expects LSB-first
      .skip_msk             = false,
    #endif
    };

    // Install driver (if already installed, treat as OK)
    esp_err_t err = i2s_driver_install(Config::Port, &cfg, 0, nullptr);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
      Serial.printf("[audio] i2s_driver_install failed: %d\n", static_cast<int>(err));
      return;
    }

    i2s_pin_config_t pins_cfg{};
    pins_cfg.bck_io_num   = I2sBclk.pin;
    pins_cfg.ws_io_num    = I2sLrc.pin;
    pins_cfg.data_out_num = I2sDin.pin;
    pins_cfg.data_in_num  = I2S_PIN_NO_CHANGE;
    pins_cfg.mck_io_num   = I2S_PIN_NO_CHANGE;

    err = i2s_set_pin(Config::Port, &pins_cfg);
    if (err != ESP_OK)
    {
      Serial.printf("[audio] i2s_set_pin failed: %d\n", static_cast<int>(err));
      return;
    }

    // Known safe state: silence
    i2s_zero_dma_buffer(Config::Port);
  }
}
