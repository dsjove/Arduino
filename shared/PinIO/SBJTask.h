#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <limits.h>

#if defined(ARDUINO_ARCH_ESP32)
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #define SBJVTask 1
#else
  #ifndef _TASK_LTS_POINTER
    #define _TASK_LTS_POINTER
  #endif
  #include <TaskScheduler.h>
  #define SBJVTask 0
#endif

#if SBJVTask
  static constexpr int32_t FOREVER = -1;
  using CoreID = BaseType_t;
#else
  static constexpr int32_t FOREVER = TASK_FOREVER;
  using CoreID = int;
#endif

enum class TaskPriority : uint8_t {
  Low    = 1,
  Medium = 2,
  High   = 3
};

class SBJTask {
public:
  enum class LoopBehavior : uint8_t {
    Yield,
    DelayTicks,
    DelayMs
  };

  template <LoopBehavior Behavior = LoopBehavior::DelayTicks, uint32_t Param = 1>
  static inline void loop() {
#if SBJVTask
    if constexpr (Behavior == LoopBehavior::Yield) {
      taskYIELD();
    } else if constexpr (Behavior == LoopBehavior::DelayTicks) {
      vTaskDelay((Param == 0) ? 1 : Param); // avoid 0 meaning "no-op"; keep loop cooperative
    } else if constexpr (Behavior == LoopBehavior::DelayMs) {
      vTaskDelay(pdMS_TO_TICKS((Param == 0) ? 1 : Param));
    } else {
      taskYIELD();
    }
#else
    (void)Behavior;
    (void)Param;
    _scheduler.execute();
#endif
  }

  using Fn0 = void (*)();

#if SBJVTask
  template <uint32_t IntervalMs, int32_t Iterations, uint32_t StartDelayMs>
  struct TimingTraits {
    // Sanity checks at compile time (C++17)
    static_assert(Iterations == FOREVER || Iterations > 0,
                  "Iterations must be FOREVER (-1) or > 0");

    static constexpr uint32_t msToTicks(uint32_t ms) {
      return (ms == 0) ? 0u
                       : ((ms + (uint32_t)portTICK_PERIOD_MS - 1u) / (uint32_t)portTICK_PERIOD_MS);
    }

    static constexpr uint32_t startDelayTicks = msToTicks(StartDelayMs);
    static constexpr uint32_t intervalTicks   = msToTicks(IntervalMs);
    static constexpr int32_t  iterations      = Iterations;
  };
#endif

  template <uint32_t IntervalMs = 1, int32_t Iterations = FOREVER, uint32_t StartDelayMs = 0>
  SBJTask(
      const char*  name,
      uint32_t     stackDepth,
      Fn0          fn,
      TaskPriority priority = TaskPriority::Low,
      CoreID       coreId   = 0)
#if SBJVTask
  : _fn0_runtime(fn)
  , _arg_runtime(nullptr)
#else
  : _task(IntervalMs, Iterations, fn, &_scheduler, true)
#endif
  {
#if SBJVTask
    using TT = TimingTraits<IntervalMs, Iterations, StartDelayMs>;
    (void)xTaskCreatePinnedToCore(
        &espEntry<TT, InvokeRuntimeFn0>,
        name,
        stackDepth,
        this,
        static_cast<UBaseType_t>(priority),
        nullptr,
        coreId);
#else
    if constexpr (StartDelayMs != 0) _task.delay(StartDelayMs);
    (void)stackDepth; (void)priority; (void)coreId; (void)name;
#endif
  }

  template <typename T, void (T::*Method)(),
            uint32_t IntervalMs = 1, int32_t Iterations = FOREVER, uint32_t StartDelayMs = 0>
  SBJTask(
      const char*  name,
      uint32_t     stackDepth,
      T*           obj,
      TaskPriority priority = TaskPriority::Low,
      CoreID       coreId   = 0)
#if SBJVTask
  : _fn0_runtime(nullptr)
  , _arg_runtime(static_cast<void*>(obj))
#else
  : _task(IntervalMs, Iterations, &taskMemWrapper<T, Method>, &_scheduler, true)
#endif
  {
#if SBJVTask
    using TT = TimingTraits<IntervalMs, Iterations, StartDelayMs>;
    (void)xTaskCreatePinnedToCore(
        &espEntry<TT, InvokeMember<T, Method>>,
        name,
        stackDepth,
        this,
        static_cast<UBaseType_t>(priority),
        nullptr,
        coreId);
#else
    _task.setLtsPointer(obj);
    if constexpr (StartDelayMs != 0) _task.delay(StartDelayMs);
    (void)stackDepth; (void)priority; (void)coreId; (void)name;
#endif
  }

  SBJTask(const SBJTask&) = delete;
  SBJTask& operator=(const SBJTask&) = delete;
  SBJTask(SBJTask&&) = delete;
  SBJTask& operator=(SBJTask&&) = delete;

private:
#if SBJVTask
  Fn0   const _fn0_runtime;  // only used by runtime-fn ctor; nullptr otherwise
  void* const _arg_runtime;  // member-method object pointer; nullptr otherwise
#else
  static inline Scheduler _scheduler;
  Task _task;
#endif

#if SBJVTask
  template <typename TT, typename Invoker>
  static void espEntry(void* pv) {
    runTimedLoop<TT, Invoker>(static_cast<SBJTask*>(pv));
    vTaskDelete(nullptr);
  }

  template <typename TT, typename Invoker>
  static inline void runTimedLoop(SBJTask* self) {
    if (!self) return;
    if (!Invoker::init(self)) return;

    constexpr uint32_t startDelayTicks = TT::startDelayTicks;
    constexpr uint32_t intervalTicks   = TT::intervalTicks;
    constexpr int32_t  iterations      = TT::iterations;

    if (startDelayTicks == 0) taskYIELD();
    else vTaskDelay(startDelayTicks);

    if constexpr (iterations == FOREVER) {
      for (;;) {
        Invoker::call(self);
        if (intervalTicks == 0) taskYIELD();
        else vTaskDelay(intervalTicks);
      }
    } else {
      for (int32_t i = 0; i < iterations; ++i) {
        Invoker::call(self);
        if (i + 1 < iterations) {
          if (intervalTicks == 0) taskYIELD();
          else vTaskDelay(intervalTicks);
        }
      }
    }
  }

  struct InvokeRuntimeFn0 {
    static inline bool init(SBJTask* self) { return self->_fn0_runtime != nullptr; }
    static inline void call(SBJTask* self) { self->_fn0_runtime(); }
  };

  template <typename T, void (T::*Method)()>
  struct InvokeMember {
    static inline bool init(SBJTask* self) { return self->_arg_runtime != nullptr; }
    static inline void call(SBJTask* self) {
      auto* obj = static_cast<T*>(self->_arg_runtime);
      (obj->*Method)();
    }
  };
#else
  template <Fn0 Fn>
  static inline void taskFn0Wrapper() { Fn(); }

  template <typename T, void (T::*Method)()>
  static inline void taskMemWrapper() {
    Task* cur = Task::getCurrentTask();
    auto* obj = cur ? static_cast<T*>(cur->getLtsPointer()) : nullptr;
    if (!obj) return;
    (obj->*Method)();
  }
#endif
};
