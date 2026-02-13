#pragma once

#include <Arduino.h>
#include "TheFS.h"

#include <SD.h>
#if defined(ARDUINO_ARCH_ESP32)
  #include <SD_MMC.h>
#endif

#include "../PinIO/PinIO.h"
#include "../PinIO/SPIHardware.h"

struct TheSDCardTraitsSPI
{
  static constexpr bool kUseMMC = false;

  using SdCs = PinIO<21, GpioMode::DigitalOut>;

  static SPIClass& spi() { return SPI; } // override if you use a different SPIClass
};

#if defined(ARDUINO_ARCH_ESP32)
struct TheSDCardTraitsMMC
{
  static constexpr bool kUseMMC = true;

  static constexpr const char* kMountPoint = "/sdcard";
  static constexpr bool        kOneBitMode = true; // safer default
};
#endif


template <typename Traits = TheSDCardTraitsSPI>
class TheSDCard
{
public:
  TheSDCard() = default;

  inline void prepare()
  {
    if constexpr (!Traits::kUseMMC)
    {
      SPIHardware::prepare<typename Traits::SdCs>();
    }
    else
    {
#if !defined(ARDUINO_ARCH_ESP32)
      static_assert(!Traits::kUseMMC, "SD_MMC is only available on ESP32 targets.");
#endif
      // No SPI prepare for SD_MMC
    }
  }

  bool begin()
  {
    bool ok = false;

    if constexpr (Traits::kUseMMC)
    {
#if defined(ARDUINO_ARCH_ESP32)
      ok = SD_MMC.begin(Traits::kMountPoint, Traits::kOneBitMode);
#else
      ok = false;
#endif
    }
    else
    {
      ok = SD.begin(Traits::SdCs::pin, Traits::spi());
    }
    _mounted = ok;
    if (ok)
    {
      TheFS::_fs = &fs();
    }
    return ok;
  }

  bool mounted() const { return _mounted; }

  fs::FS& fs()
  {
    if constexpr (Traits::kUseMMC)
    {
#if defined(ARDUINO_ARCH_ESP32)
      return SD_MMC;
#else
      return SD;
#endif
    }
    else
    {
      return SD;
    }
  }

  File open(const char* path, const char* mode = FILE_READ)
  {
    return fs().open(path, mode);
  }

  void debugPrint(Stream& out = Serial)
  {
    listRoot(out);
  }

  void listRoot(Stream& out = Serial)
  {
    if (!_mounted)
    {
      out.println("SD not mounted");
      return;
    }

    File root = fs().open("/");
    if (!root || !root.isDirectory())
    {
      out.println("Failed to open root");
      return;
    }

    out.println("Root directory:");

    for (File file = root.openNextFile(); file; file = root.openNextFile())
    {
      out.print("  ");
      out.print(file.name());

      if (file.isDirectory())
      {
        out.println("  <DIR>");
      }
      else
      {
        out.print("  ");
        out.print(file.size());
        out.println(" bytes");
      }
    }

    root.close();
  }

private:
  bool _mounted = false;
};
