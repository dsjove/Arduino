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
    SchedulerState::scheduler.execute();
#endif
  }

  struct Schedule
  {
    static constexpr int32_t kForever = FOREVER;
    const uint32_t     intervalMs;
    const int32_t      iterations;
    const uint32_t     startDelayMs;
    const uint32_t     stackDepth;
    const TaskPriority priority;
    const CoreID       coreId;

    Schedule(uint32_t intervalMs_   = 1,
             int32_t iterations_    = kForever,
             uint32_t startDelayMs_ = 0,
             uint32_t stackDepth_   = 4096,
             TaskPriority priority_ = TaskPriority::Low,
             CoreID coreId_         = 0)
    : intervalMs(intervalMs_)
    , iterations(iterations_)
    , startDelayMs(startDelayMs_)
    , stackDepth(stackDepth_)
    , priority(priority_)
    , coreId(coreId_)
    {
#ifndef NDEBUG
      if (intervalMs_ == 0) { /* invalid interval */ }
      if (!(iterations_ == kForever || iterations_ > 0)) { /* invalid iterations */ }
#endif
    }
  };

  using Fn0 = void (*)();

  // ============================================================
  // Runtime global function constructor
  // ============================================================
  SBJTask(const char* name, Fn0 fn, Schedule schedule = Schedule{})
#if SBJVTask
  : _esp(EspState::makeRuntime(name, schedule, fn))
#else
  : _scheduler(SchedulerState::makeRuntime(name, schedule, fn))
#endif
  {}

  // ============================================================
  // Member method constructor
  // ============================================================
  template <typename T, void (T::*Method)()>
  SBJTask(const char* name, T* obj, Schedule schedule = Schedule{})
#if SBJVTask
  : _esp(EspState::template makeMember<T, Method>(name, schedule, obj))
#else
  : _scheduler(SchedulerState::template makeMember<T, Method>(name, schedule, obj))
#endif
  {}

  // ============================================================
  // Descriptor constructor (expects Desc::schedule + Desc::Method)
  // ============================================================
  template <typename Desc>
  SBJTask(const char* name, typename Desc::Obj* obj, Desc = Desc{})
#if SBJVTask
  : _esp(EspState::template makeMember<typename Desc::Obj, Desc::Method>(name, Desc::schedule, obj))
#else
  : _scheduler(SchedulerState::template makeMember<typename Desc::Obj, Desc::Method>(name, Desc::schedule, obj))
#endif
  {}

  inline bool begun() const
  {
#if SBJVTask
    return _esp.begun;
#else
    return _scheduler.task.isEnabled();
#endif
  }

  inline void begin()
  {
    if (begun()) return;
#if SBJVTask
    BaseType_t ok = xTaskCreatePinnedToCore(
        _esp.entry,
        _esp.name ? _esp.name : "SBJTask",
        _esp.stackDepth,
        this,
        _esp.priority,
        &_esp.handle,
        _esp.coreId);

    if (ok == pdPASS) {
      _esp.begun = true;
    }
#else
    _scheduler.task.enable();
#endif
  }

  SBJTask(const SBJTask&) = delete;
  SBJTask& operator=(const SBJTask&) = delete;
  SBJTask(SBJTask&&) = delete;
  SBJTask& operator=(SBJTask&&) = delete;

private:
#if SBJVTask
  struct EspState {
    using InitFn = bool (*)(SBJTask*);
    using CallFn = void (*)(SBJTask*);
    const InitFn initFn;
    const CallFn callFn;
    const TaskFunction_t entry;
    const char*          name;
    const Fn0            fn0;
    const void*          arg;
    const uint32_t       stackDepth;
    const UBaseType_t    priority;
    const CoreID         coreId;
    const int32_t        iterations;
    const TickType_t     intervalTicks;
    const TickType_t     startDelayTicks;
    bool                 begun;
    TaskHandle_t         handle;

    static inline bool initRuntime(SBJTask* self)
    {
      return self && self->_esp.fn0 != nullptr;
    }

    static inline void callRuntime(SBJTask* self)
    {
      self->_esp.fn0();
    }

    template <typename T, void (T::*Method)()>
    static inline bool initMember(SBJTask* self)
    {
      return self && self->_esp.arg != nullptr;
    }

    template <typename T, void (T::*Method)()>
    static inline void callMember(SBJTask* self)
    {
      auto* obj = static_cast<T*>(const_cast<void*>(self->_esp.arg));
      (obj->*Method)();
    }

    static void entryThunk(void* pv)
    {
      SBJTask* self = static_cast<SBJTask*>(pv);
      if (!self) { vTaskDelete(nullptr); return; }

      if (!self->_esp.initFn || !self->_esp.callFn) { vTaskDelete(nullptr); return; }
      if (!self->_esp.initFn(self)) { vTaskDelete(nullptr); return; }

      if (self->_esp.startDelayTicks == 0) taskYIELD();
      else vTaskDelay(self->_esp.startDelayTicks);

      if (self->_esp.iterations == FOREVER) {
        for (;;) {
          self->_esp.callFn(self);
          if (self->_esp.intervalTicks == 0) taskYIELD();
          else vTaskDelay(self->_esp.intervalTicks);
        }
      } else {
        const int32_t iters = self->_esp.iterations;
        for (int32_t i = 0; i < iters; ++i) {
          self->_esp.callFn(self);
          if (i + 1 < iters) {
            if (self->_esp.intervalTicks == 0) taskYIELD();
            else vTaskDelay(self->_esp.intervalTicks);
          }
        }
      }

      vTaskDelete(nullptr);
    }

    EspState(const char* n,
             const Schedule& s,
             const void* a,
             Fn0 f,
             InitFn initFn_,
             CallFn callFn_)
    : initFn(initFn_)
    , callFn(callFn_)
    , entry(&EspState::entryThunk)
    , name(n)
    , fn0(f)
    , arg(a)
    , stackDepth(s.stackDepth)
    , priority(static_cast<UBaseType_t>(s.priority))
    , coreId(s.coreId)
    , iterations(s.iterations)
    , intervalTicks(pdMS_TO_TICKS(s.intervalMs))
    , startDelayTicks(pdMS_TO_TICKS(s.startDelayMs))
    , begun(false)
    , handle(nullptr)
    {}

    static inline EspState makeRuntime(const char* n, const Schedule& s, Fn0 f)
    {
      return EspState(n, s, nullptr, f, &EspState::initRuntime, &EspState::callRuntime);
    }

    template <typename T, void (T::*Method)()>
    static inline EspState makeMember(const char* n, const Schedule& s, T* obj)
    {
      return EspState(n,
                      s,
                      static_cast<const void*>(obj),
                      nullptr,
                      &EspState::template initMember<T, Method>,
                      &EspState::template callMember<T, Method>);
    }
  } _esp;

#else
  struct SchedulerState {
    static inline Scheduler scheduler;
    Task task;

    template <typename T, void (T::*Method)()>
    static inline void memberThunk()
    {
      Task* cur = Task::getCurrentTask();
      auto* obj = cur ? static_cast<T*>(cur->getLtsPointer()) : nullptr;
      if (obj) (obj->*Method)();
    }

    SchedulerState(uint32_t intervalMs,
                   int32_t  iterations,
                   uint32_t startDelayMs,
                   Fn0      fn,
                   void*    ltsObj)
    : task(intervalMs, iterations, fn, &scheduler, false)
    {
      if (startDelayMs != 0) task.delay(startDelayMs);
      if (ltsObj != nullptr) task.setLtsPointer(ltsObj);
    }

    static inline SchedulerState makeRuntime(const char* /*name*/, const Schedule& s, Fn0 fn)
    {
      return SchedulerState(s.intervalMs, s.iterations, s.startDelayMs, fn, nullptr);
    }

    template <typename T, void (T::*Method)()>
    static inline SchedulerState makeMember(const char* /*name*/, const Schedule& s, T* obj)
    {
      return SchedulerState(s.intervalMs,
                            s.iterations,
                            s.startDelayMs,
                            &SchedulerState::template memberThunk<T, Method>,
                            static_cast<void*>(obj));
    }
  } _scheduler;
#endif
};
