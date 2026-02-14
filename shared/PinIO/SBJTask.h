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
  static inline void loop()
  {
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
    SchedState::scheduler.execute();
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
                       : ((ms + (uint32_t)portTICK_PERIOD_MS - 1u)
                          / (uint32_t)portTICK_PERIOD_MS);
    }
    static constexpr uint32_t intervalTicks   = msToTicks(IntervalMs_);
    static constexpr int32_t  iterations      = Iterations_;
    static constexpr uint32_t startDelayTicks = msToTicks(StartDelayMs_);
#endif
    static constexpr uint32_t IntervalMs   = IntervalMs_;
    static constexpr int32_t  Iterations   = Iterations_;
    static constexpr uint32_t StartDelayMs = StartDelayMs_;
  };

  // ============================================================
  // Runtime global function constructor
  // ============================================================
  template <typename Timing = TimingTraits<1, FOREVER, 0>>
  SBJTask(
      const char*  name,
      uint32_t     stackDepth,
      Fn0          fn,
      TaskPriority priority = TaskPriority::Low,
      CoreID       coreId   = 0)
  : _begun(false)
#if SBJVTask
  , _esp(name, stackDepth, priority, coreId)
#else
  , _sched{ Timing::IntervalMs, Timing::Iterations, Timing::StartDelayMs,
            fn, nullptr }
#endif
  {
#if SBJVTask
  _esp.template bindEntry<Timing, InvokeRuntimeFn0>(nullptr, fn);
#else
    ignoreUnused(name, stackDepth, priority, coreId);
#endif
  }

  // ============================================================
  // Member method constructor
  // ============================================================
  template <typename T, void (T::*Method)(),
            typename Timing = TimingTraits<1, FOREVER, 0>>
  SBJTask(
      const char*  name,
      uint32_t     stackDepth,
      T*           obj,
      TaskPriority priority = TaskPriority::Low,
      CoreID       coreId   = 0)
  : _begun(false)
#if SBJVTask
  , _esp(name, stackDepth, priority, coreId)
#else
  , _sched{ Timing::IntervalMs, Timing::Iterations, Timing::StartDelayMs,
            &taskMemWrapper<T, Method>, static_cast<void*>(obj) }
#endif
  {
#if SBJVTask
  _esp.template bindEntry<Timing, InvokeMember<T, Method>>(
    nullptr, static_cast<void*>(obj));
#else
    ignoreUnused(name, stackDepth, priority, coreId);
#endif
  }

  // ============================================================
  // Descriptor constructor
  // ============================================================
  template <typename Desc>
  SBJTask(
      const char*  name,
      uint32_t     stackDepth,
      typename Desc::Obj* obj,
      TaskPriority priority = TaskPriority::Low,
      CoreID       coreId   = 0,
      Desc         = Desc{})
  : _begun(false)
#if SBJVTask
  , _esp(name, stackDepth, priority, coreId)
#else
  , _sched{ Desc::Timing::IntervalMs, Desc::Timing::Iterations, Desc::Timing::StartDelayMs,
            &taskMemWrapper<typename Desc::Obj, Desc::Method>, static_cast<void*>(obj) }
#endif
  {
#if SBJVTask
  _esp.template bindEntry<typename Desc::Timing, InvokeMember<typename Desc::Obj, Desc::Method>>(
    static_cast<void*>(obj), nullptr);
#else
    ignoreUnused(name, stackDepth, priority, coreId);
#endif
  }

  inline bool begun() const { return _begun; }

  inline void begin() {
    if (_begun) return;
#if SBJVTask
    if (_esp.entry == nullptr) return;
    (void)xTaskCreatePinnedToCore(
        _esp.entry,
        _esp.name ? _esp.name : "SBJTask",
        _esp.stackDepth,
        this,
        _esp.priority,
        &_esp.handle,
        _esp.coreId);
#else
    _sched.task.enable();
#endif
    _begun = true;
  }

  SBJTask(const SBJTask&) = delete;
  SBJTask& operator=(const SBJTask&) = delete;
  SBJTask(SBJTask&&) = delete;
  SBJTask& operator=(SBJTask&&) = delete;

private:
  bool _begun;

#if SBJVTask
  struct EspState {
    Fn0            fn0        = nullptr;
    void*          arg        = nullptr;
    const char*    name       = nullptr;
    uint32_t       stackDepth = 0;
    UBaseType_t    priority   = 0;
    CoreID         coreId     = 0;
    TaskFunction_t entry      = nullptr;
    TaskHandle_t   handle     = nullptr;

    EspState() = default;

    EspState(const char* n, uint32_t sd, TaskPriority pr, CoreID cid)
    : fn0(nullptr)
    , arg(nullptr)
    , name(n)
    , stackDepth(sd)
    , priority(static_cast<UBaseType_t>(pr))
    , coreId(cid)
    , entry(nullptr)
    , handle(nullptr)
    {}

    template <typename Timing, typename Invoker>
    inline void bindEntry(void* a, Fn0 f) {
      fn0 = f;
      arg = a;
      entry = &SBJTask::espEntry<Timing, Invoker>;
    }
  } _esp;

#else
  struct SchedState {
    static inline Scheduler scheduler;
    Task task;

    SchedState(uint32_t intervalMs,
               int32_t  iterations,
               uint32_t startDelayMs,
               Fn0      fn,
               void*    ltsObj)
    : task(intervalMs, iterations, fn, &scheduler, false)
    {
      if (startDelayMs != 0) task.delay(startDelayMs);
      if (ltsObj != nullptr) task.setLtsPointer(ltsObj);
    }
  } _sched;

  inline void ignoreUnused(const char* name,
                           uint32_t stackDepth,
                           TaskPriority priority,
                           CoreID coreId) {
    (void)name; (void)stackDepth; (void)priority; (void)coreId;
  }
#endif

#if SBJVTask
  template <typename Timing, typename Invoker>
  static void espEntry(void* pv) {
    runTimedLoop<Timing, Invoker>(
        static_cast<SBJTask*>(pv));
    vTaskDelete(nullptr);
  }

  template <typename Timing, typename Invoker>
  static inline void runTimedLoop(SBJTask* self) {
    if (!self) return;
    if (!Invoker::init(self)) return;

    constexpr uint32_t startDelayTicks = Timing::startDelayTicks;
    constexpr uint32_t intervalTicks   = Timing::intervalTicks;
    constexpr int32_t  iterations      = Timing::iterations;

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
    static inline bool init(SBJTask* self) {
      return self && self->_esp.fn0 != nullptr;
    }
    static inline void call(SBJTask* self) {
      self->_esp.fn0();
    }
  };

  template <typename T, void (T::*Method)()>
  struct InvokeMember {
    using Obj = T;
    static inline bool init(SBJTask* self) {
      return self && self->_esp.arg != nullptr;
    }
    static inline void call(SBJTask* self) {
      auto* obj = static_cast<Obj*>(self->_esp.arg);
      (obj->*Method)();
    }
  };

#else
  template <typename T, void (T::*Method)()>
  static inline void taskMemWrapper() {
    Task* cur = Task::getCurrentTask();
    auto* obj = cur ? static_cast<T*>(cur->getLtsPointer()) : nullptr;
    if (obj) (obj->*Method)();
  }
#endif
};
