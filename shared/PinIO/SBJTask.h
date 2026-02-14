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
      vTaskDelay((Param == 0) ? 1 : Param);
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

  template <uint32_t IntervalMs_, int32_t Iterations_, uint32_t StartDelayMs_>
  struct TimingTraits {
    static_assert(Iterations_ == FOREVER || Iterations_ > 0,
                  "Iterations must be FOREVER (-1) or > 0");
#if SBJVTask
    static constexpr uint32_t msToTicks(uint32_t ms) {
      return (ms == 0) ? 0u
                       : ((ms + (uint32_t)portTICK_PERIOD_MS - 1u) / (uint32_t)portTICK_PERIOD_MS);
    }
    static constexpr uint32_t intervalTicks   = msToTicks(IntervalMs_);
    static constexpr int32_t  iterations      = Iterations_;
    static constexpr uint32_t startDelayTicks = msToTicks(StartDelayMs_);
#endif
    static constexpr uint32_t IntervalMs   = IntervalMs_;
    static constexpr int32_t  Iterations   = Iterations_;
    static constexpr uint32_t StartDelayMs = StartDelayMs_;
  };

  // begin(): deferred start
  inline void begin() {
    if (_begun) return;
#if SBJVTask
    if (_entry == nullptr) return;
    (void)xTaskCreatePinnedToCore(
        _entry,
        _name ? _name : "SBJTask",
        _stackDepth,
        this,
        _priority,
        &_handle,
        _coreId);
#else
    _task.enable();
#endif
    _begun = true;
  }

  inline bool begun() const { return _begun; }

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
  , _name(name)
  , _stackDepth(stackDepth)
  , _priority(static_cast<UBaseType_t>(priority))
  , _coreId(coreId)
  , _entry(nullptr)
  , _handle(nullptr)
  , _begun(false)
#else
  : _task(IntervalMs, Iterations, fn, &_scheduler, false)
  , _begun(false)
#endif
  {
#if SBJVTask
    using TT = TimingTraits<IntervalMs, Iterations, StartDelayMs>;
    _entry = &espEntry<TT, InvokeRuntimeFn0>;
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
  , _name(name)
  , _stackDepth(stackDepth)
  , _priority(static_cast<UBaseType_t>(priority))
  , _coreId(coreId)
  , _entry(nullptr)
  , _handle(nullptr)
  , _begun(false)
#else
  : _task(IntervalMs, Iterations, &taskMemWrapper<T, Method>, &_scheduler, false)
  , _begun(false)
#endif
  {
#if SBJVTask
    using TT = TimingTraits<IntervalMs, Iterations, StartDelayMs>;
    _entry = &espEntry<TT, InvokeMember<T, Method>>;
#else
    _task.setLtsPointer(obj);
    if constexpr (StartDelayMs != 0) _task.delay(StartDelayMs);
    (void)stackDepth; (void)priority; (void)coreId; (void)name;
#endif
  }

  // Descriptor-based constructor
  template <typename Desc>
  SBJTask(
      const char*  name,
      uint32_t     stackDepth,
      typename Desc::Obj* obj,
      TaskPriority priority = TaskPriority::Low,
      CoreID       coreId   = 0,
      Desc         = Desc{})
#if SBJVTask
  : _fn0_runtime(nullptr)
  , _arg_runtime(static_cast<void*>(obj))
  , _name(name)
  , _stackDepth(stackDepth)
  , _priority(static_cast<UBaseType_t>(priority))
  , _coreId(coreId)
  , _entry(nullptr)
  , _handle(nullptr)
  , _begun(false)
#else
  : _task(Desc::Timing::IntervalMs,
          Desc::Timing::Iterations,
          &taskMemWrapper<typename Desc::Obj, Desc::Method>,
          &_scheduler,
          false)
  , _begun(false)
#endif
  {
#if SBJVTask
    using TT = typename Desc::Timing;
    _entry = &espEntry<TT, InvokeMember<typename Desc::Obj, Desc::Method>>;
#else
    _task.setLtsPointer(static_cast<void*>(obj));
    if constexpr (Desc::Timing::StartDelayMs != 0) _task.delay(Desc::Timing::StartDelayMs);
    (void)stackDepth; (void)priority; (void)coreId; (void)name;
#endif
  }

  SBJTask(const SBJTask&) = delete;
  SBJTask& operator=(const SBJTask&) = delete;
  SBJTask(SBJTask&&) = delete;
  SBJTask& operator=(SBJTask&&) = delete;

private:
#if SBJVTask
  Fn0   const _fn0_runtime;   // only used by runtime-fn ctor; nullptr otherwise
  void* const _arg_runtime;   // member-method object pointer; nullptr otherwise

  // Deferred creation state
  const char*     _name;
  uint32_t        _stackDepth;
  UBaseType_t     _priority;
  CoreID          _coreId;
  TaskFunction_t  _entry;
  TaskHandle_t    _handle;
  bool            _begun;
#else
  static inline Scheduler _scheduler;
  Task _task;
  bool _begun;
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
