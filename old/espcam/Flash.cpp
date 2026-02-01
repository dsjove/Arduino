#include "Flash.h"
#include "esp32-hal-ledc.h"

static int led_pin = -1;
static bool led_enabled = false;
static int led_duty = -1;

void Flash::begin(int pin) {
  if (pin != -1) {
    led_pin = pin;
    led_enabled = true;
    led_duty = 0;
    ledcAttach(led_pin, 5000, 8);
  }
}

int Flash::getIntensity() {
  return led_pin;
}

void Flash::setIntensity(int val) {
  if (led_pin < 0) return;

  if (val > 255) {
    val = 255;
  }
  else if (val < 0) {
    val = 0;
  }
  led_duty = val;
  if (led_enabled) {
    ledcWrite(led_pin, led_duty);
  }
}

void Flash::setEnabled(bool enabled) {
  if (enabled) {
    if (led_pin != -1) {
      led_enabled = true;
      ledcWrite(led_pin, led_duty);
      //ledc_set_duty(CONFIG_LED_LEDC_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL, duty);
      //ledc_update_duty(CONFIG_LED_LEDC_SPEED_MODE, CONFIG_LED_LEDC_CHANNEL);
    }
  }
  else {
    //led_enabled = false;
    if (led_pin != -1) {
      //ledcWrite(led_pin, 0);
    }
  }
}

Flash::On::On(bool enable)
: _enable(enable) {
  // The LED needs to be turned on ~150ms before the call to esp_camera_fb_get()
  if (_enable && !led_enabled && led_pin != -1) {
    vTaskDelay(150 / portTICK_PERIOD_MS);
    ledcWrite(led_pin, led_duty);
  }
}

Flash::On::~On() {
  if (_enable && !led_enabled && led_pin != -1) {
    ledcWrite(led_pin, 0);
  }
}
