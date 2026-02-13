#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "../pinio/PinIO.h"
#include "../pinio/SPIHardware.h"

#include <Adafruit_ST7789.h>
#include <stdint.h>

struct ST7789TraitsDft
{
  using Device = Adafruit_ST7789;

  using Dc = PinIO<D1, GpioMode::DigitalOut>; // Blue

  //SPI
  //DIN - Green - MOSI - D10
  //Clock - Orange - SCK - D8
  using Cs = PinIO<D3, GpioMode::DigitalOut>; // Yellow

  //Power
  //VCC - purple - v5
  //Ground - white - GND
  //Backlight gray - v3.3
  using Rst = PinIO<D0, GpioMode::DigitalOut>; // Brown

  inline static constexpr uint16_t Width  = 320;
  inline static constexpr uint16_t Height = 240;
  inline static constexpr uint8_t Rotation = 0;
  inline static constexpr uint8_t XOffset = 0;
  inline static constexpr uint8_t YOffset = 0;
  inline static constexpr uint16_t BootFillColor = ST77XX_GREEN;
};

template <class Traits = ST7789TraitsDft>
struct ST7789Display
{
  using Device = typename Traits::Device;

  inline static Device device{
    Traits::Cs::pin,
    Traits::Dc::pin,
    Traits::Rst::pin
  };

  inline void prepare()
  {
    SPIHardware::prepare<typename Traits::Cs>();
    Traits::Dc::begin(GpioLevel::Low);
    Traits::Rst::begin(GpioLevel::High);
delay(10);
    Traits::Rst::begin(GpioLevel::Low);
delay(20);
    Traits::Rst::begin(GpioLevel::High);
delay(120);
  }

  inline bool begin()
  {
    // Panel
    device.init(Traits::Width, Traits::Height);
    device.setRotation(Traits::Rotation);
    if constexpr ((Traits::XOffset != 0) || (Traits::YOffset != 0))
    {
      device.setRowColStart(Traits::XOffset, Traits::YOffset);
    }
    device.fillScreen(Traits::BootFillColor);
    return true;
  }

// Convert RGB888 to RGB565
uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return (r & 0xF8) << 8 | (g & 0xFC) << 3 | (b >> 3);
}

uint16_t read16(File &f) {
  uint16_t v;
  ((uint8_t*)&v)[0] = f.read();
  ((uint8_t*)&v)[1] = f.read();
  return v;
}

uint32_t read32(File &f) {
  uint32_t v;
  ((uint8_t*)&v)[0] = f.read();
  ((uint8_t*)&v)[1] = f.read();
  ((uint8_t*)&v)[2] = f.read();
  ((uint8_t*)&v)[3] = f.read();
  return v;
}

bool drawBMP(const char *filename, int x = 0, int y = 0) {
  File bmpFile = SD.open(filename);
  if (!bmpFile) { Serial.println("BMP not found"); return false; }

  if (read16(bmpFile) != 0x4D42) { Serial.println("Not BMP"); bmpFile.close(); return false; }

  read32(bmpFile); // size
  read32(bmpFile); // creator
  uint32_t offset = read32(bmpFile);
  read32(bmpFile); // DIB header
  int32_t width  = (int32_t)read32(bmpFile);
  int32_t height = (int32_t)read32(bmpFile);

  if (read16(bmpFile) != 1) { Serial.println("Init read failed"); bmpFile.close(); return false; }
  uint16_t depth = read16(bmpFile);
  uint32_t compression = read32(bmpFile);

  if (depth != 24 || compression != 0) {
    Serial.println("Unsupported BMP format");
    bmpFile.close();
    return false;
  }

  uint32_t rowSize = (width * 3 + 3) & ~3;
  bool flip = true;
  if (height < 0) { height = -height; flip = false; }

  // set the drawing region once
  device.startWrite();
  device.setAddrWindow(x, y, width, height);
  device.endWrite();

  uint16_t lineBuffer[80];

  for (int row = 0; row < height; row++) {
    uint32_t pos = offset + (flip ? (height - 1 - row) : row) * rowSize;
    bmpFile.seek(pos);

    int col = 0;
    while (col < width) {
      int chunk = min((long)80, (long)(width - col));

      // ---- SD SPI only ----
      for (int i = 0; i < chunk; i++) {
        uint8_t b = bmpFile.read();
        uint8_t g = bmpFile.read();
        uint8_t r = bmpFile.read();
        lineBuffer[i] = rgb565(r, g, b);
      }

      // ---- TFT SPI only ----
      device.startWrite();
      device.writePixels(lineBuffer, chunk, true);
      device.endWrite();

      col += chunk;
    }
  }

  bmpFile.close();
  return true;
}


};
/*
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// ======== PIN CONFIG ========
#define TFT_CS   7
#define TFT_DC   3
#define TFT_RST  2

#define SD_CS    10

#define TFT_W    240
#define TFT_H    320

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  Serial.begin(115200);

  SPI.begin();          // or specify SCK, MISO, MOSI if remapped
  tft.init(TFT_W, TFT_H);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);

  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed");
    return;
  }

  drawBMP("/image.bmp", 0, 0);
}

void loop() {}
*/
