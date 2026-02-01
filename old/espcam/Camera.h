#pragma once

#include "Flash.h"
#include <esp_camera.h>
#include <img_converters.h>
#include <limits>

class ESP32Config;

class Camera {
public:
  Camera();

  bool begin(ESP32Config& config);

  static uint16_t getPID();

  static bool setValue(const char* variable, int val);

  static void getValues(char json_response[1024]);

  static int setXLCK(int val);

  static int setReg(int reg, int mask, int val);

  static int getReg(int reg, int mask);

  class Frame {
  public:
    inline Frame(bool flash) 
    : _flash(flash), fb(esp_camera_fb_get()), timestamp(fb ? fb->timestamp : timeval{0, 0}) {}
    inline ~Frame() { consume(); }
    inline operator bool () const { return fb != NULL; }
    inline void consume() {if (fb) { esp_camera_fb_return(fb); fb = NULL; } }

    static const size_t SingleIndex;
    static const size_t End;

    bool jpg(
        jpg_out_cb iterate, void* capture, 
        bool allAtOnce, int quality, 
        bool consumeFrame = true);

  private:
    Flash::On _flash;
  public:
    camera_fb_t* fb;
    const struct timeval timestamp;
  };

  class BMP {
  public:
    inline BMP(Frame& frame, bool consumeFrame = true) 
    : _buffer(NULL)
    , _length(0)
    , _converted(frame.fb ? frame2bmp(frame.fb, &_buffer, &_length) : false) 
    { if (consumeFrame) frame.consume(); }
    
    inline ~BMP() { if (_buffer) free(_buffer); }

    inline operator bool () const { return _converted; }
    inline const uint8_t* buffer() const { return _buffer; }
    inline size_t length() const { return _length; }
  private:
    uint8_t* _buffer;
    size_t _length;
    const bool _converted;
  };

  struct PLL {
    int bypass;
    int mul;
    int sys; 
    int root;
    int pre;
    int seld5;
    int pclken;
    int pclk;
  };

  static int setPLL(const PLL& pll);

  struct Resolution {
    int startX;
    int startY;
    int endX;
    int endY;
    int offsetX;
    int offsetY;
    int totalX;
    int totalY;
    int outputX;
    int outputY;
    bool scale;
    bool binning;
  };

  static int setResolution(const Resolution& res);
};
