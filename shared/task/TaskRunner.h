#pragma once
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct TaskSpec {
  const char* name;        // e.g. "cam"
  uint32_t stack_words;    // FreeRTOS stack size in *words* (ESP32 Arduino)
  UBaseType_t priority;    // higher = more priority
  BaseType_t core;         // 0 or 1
  uint32_t period_ms;      // loop delay
};

template <typename Bus>
struct ServiceFn {
  void* self{};
  void (*begin)(void*, Bus&){};
  void (*tick)(void*, Bus&){};

  template <typename T>
  static ServiceFn make(T& obj) {
    ServiceFn s;
    s.self  = static_cast<void*>(&obj);
    s.begin = [](void* p, Bus& b) { static_cast<T*>(p)->begin(b); };
    s.tick  = [](void* p, Bus& b) { static_cast<T*>(p)->tick(b);  };
    return s;
  }
};

template <typename Bus, size_t N>
class TaskRunner {
public:
  TaskRunner(const TaskSpec& spec, Bus& bus)
  : spec_(spec), bus_(bus) {}

  bool add(ServiceFn<Bus> s) {
    if (count_ >= N) return false;
    services_[count_++] = s;
    return true;
  }

  void start() {
    xTaskCreatePinnedToCore(&TaskRunner::entry,
                            spec_.name,
                            spec_.stack_words,
                            this,
                            spec_.priority,
                            &handle_,
                            spec_.core);
  }

  TaskHandle_t handle() const { return handle_; }
  size_t count() const { return count_; }

private:
  static void entry(void* arg) {
    static_cast<TaskRunner*>(arg)->run();
  }

  void run() {
    // Begin phase
    for (size_t i = 0; i < count_; ++i) {
      if (services_[i].begin) services_[i].begin(services_[i].self, bus_);
      // small yield between begins (optional)
      taskYIELD();
    }

    const TickType_t delay = pdMS_TO_TICKS(spec_.period_ms);

    // Tick loop
    for (;;) {
      for (size_t i = 0; i < count_; ++i) {
        services_[i].tick(services_[i].self, bus_);
      }
      vTaskDelay(delay);
    }
  }

  TaskSpec spec_;
  Bus& bus_;
  TaskHandle_t handle_{nullptr};

  ServiceFn<Bus> services_[N]{};
  size_t count_{0};
};

template <typename T>
inline bool queue_overwrite_latest(QueueHandle_t q, const T& value) {
  // drop one if present
  T old{};
  xQueueReceive(q, &old, 0);
  return xQueueSend(q, &value, 0) == pdTRUE;
}

template <typename T>
inline bool queue_try_receive(QueueHandle_t q, T& out) {
  return xQueueReceive(q, &out, 0) == pdTRUE;
}

#include "freertos/queue.h"
//Make this your “wiring harness” between tasks. Start empty and add queues as needed.
struct Bus {
  // Add queues, ring buffers, shared state here.
  // Example:
  // QueueHandle_t latest_frame_q = nullptr; // camera_fb_t*
  // QueueHandle_t ble_cmd_q      = nullptr; // BleCommand
  // QueueHandle_t sample_q       = nullptr; // SensorSample
};

/*
struct FooService {
  void begin(Bus&) {}
  void tick(Bus&)  {}
};

Bus bus;
FooService foo;

TaskRunner<Bus, 1> appTask({ "app", 4096, 2, 0, 50 }, bus);

void setup() {
  appTask.add(ServiceFn<Bus>::make(foo));
  appTask.start();
}

void loop() { vTaskDelay(pdMS_TO_TICKS(1000)); }
 */
