#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <limits.h>
#include <type_traits>

#if defined(ARDUINO_ARCH_ESP32)
  #define SBJVTask 1
#else
  #define SBJVTask 0
#endif

#if SBJVTask
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  static constexpr int32_t FOREVER = -1;
  using CoreID = BaseType_t;
#else
  #ifndef _TASK_LTS_POINTER
    #define _TASK_LTS_POINTER
  #endif
  #include <TaskScheduler.h>
  static constexpr int32_t FOREVER = TASK_FOREVER;
  using CoreID = int;
#endif

/**
 * TaskPriority
 *
 * Notes:
 * - ESP32: value is cast directly to the FreeRTOS task priority argument passed to
 *   xTaskCreatePinnedToCore(). These are intentionally small, conservative defaults.
 * - non-ESP32: ignored (TaskScheduler does not use priorities).
 *
 * If you need a different mapping (e.g., based on configMAX_PRIORITIES),
 * change these values or add a ctor overload that accepts a raw priority.
 */
enum class TaskPriority : uint8_t {
  Low    = 1,
  Medium = 2,
  High   = 3
};

/**
 * LoopBehavior controls what SBJTaskLoop::loop() does on ESP32.
 *
 * Non-ESP32: loop() always executes the shared TaskScheduler.
 */
enum class LoopBehavior : uint8_t {
  SchedulerOnly, // ESP32: yield/no-op (tasks run independently)
  Yield,         // ESP32: taskYIELD()
  DelayTicks,    // ESP32: vTaskDelay(N ticks)
  DelayMs        // ESP32: vTaskDelay(pdMS_TO_TICKS(N ms))
};

namespace sbj_detail {

// ---------- member pointer introspection (supports void (C::*)() only) ----------
template <typename>
struct member_class;

template <typename C, typename R>
struct member_class<R (C::*)()> {
  using type = C;
  using ret  = R;
};

template <typename C, typename R>
struct member_class<R (C::*)() const> {
  using type = const C;
  using ret  = R;
};

template <typename T>
inline constexpr bool is_void_v = std::is_same_v<T, void>;

// ---------- shared TaskScheduler instance for all tasks (non-ESP32) ----------
#if !SBJVTask
struct SchedulerSingleton {
  static inline Scheduler scheduler{};
};
#endif

} // namespace sbj_detail

/**
 * SBJTaskLoop
 *
 * Call this from Arduino loop():
 *   void loop() { SBJTaskLoop::loop<>(); }
 *
 * On ESP32 this just cooperatively yields/delays.
 * On non-ESP32 it always drives the shared TaskScheduler.
 */
struct SBJTaskLoop {
  template <LoopBehavior Behavior = LoopBehavior::DelayTicks, uint32_t Param = 1>
  static inline void loop() {
#if SBJVTask
    if constexpr (Behavior == LoopBehavior::SchedulerOnly) {
      taskYIELD();
    } else if constexpr (Behavior == LoopBehavior::Yield) {
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
    sbj_detail::SchedulerSingleton::scheduler.execute();
#endif
  }
};

/**
 * SBJTask
 *
 * Templated task wrapper:
 *   - Callable is either a free/static function pointer: void(*)()
 *     or a member method pointer: void (T::*)()
 *   - Timing is compile-time via template params.
 *   - Iterations defaults to FOREVER.
 *
 * Free function usage:
 *   static void tick();
 *   static inline SBJTask<&MyClass::tick, 100> t{"name", 4096};
 *
 * Member method usage:
 *   void tick();
 *   SBJTask<&MyType::tick, 20> t{"name", 4096, this};
 *
 * Notes:
 * - ESP32: constructor starts a FreeRTOS task immediately.
 * - non-ESP32: constructor registers a TaskScheduler task.
 */
template <auto Callable, uint32_t IntervalMs = 1, int32_t Iterations = FOREVER, uint32_t StartDelayMs = 0>
class SBJTask {
public:
  using Fn0 = void (*)();

  static_assert(Iterations == FOREVER || Iterations > 0,
                "Iterations must be FOREVER (-1) or > 0");

  // Enforce supported callable signatures.
  static constexpr bool kIsMember = std::is_member_function_pointer_v<decltype(Callable)>;
  static constexpr bool kIsFn0    = std::is_pointer_v<decltype(Callable)>;

  // If it's a free/static function pointer, require void(*)()
  static_assert(!kIsFn0 || std::is_same_v<decltype(Callable), Fn0>,
                "Callable must be void(*)() for free/static function tasks");

  // If it's a member function pointer, require void (C::*)() or void (C::*)() const
  static_assert(!kIsMember || sbj_detail::is_void_v<typename sbj_detail::member_class<decltype(Callable)>::ret>,
                "Callable must be a member method returning void and taking no args");

#if SBJVTask
  static constexpr uint32_t msToTicks(uint32_t ms) {
    return (ms == 0) ? 0u
                     : ((ms + (uint32_t)portTICK_PERIOD_MS - 1u) / (uint32_t)portTICK_PERIOD_MS);
  }
  static constexpr uint32_t startDelayTicks = msToTicks(StartDelayMs);
  static constexpr uint32_t intervalTicks   = msToTicks(IntervalMs);
#endif

  // ---------------------------------------------------------------------------
  // Constructors
  // ---------------------------------------------------------------------------

  // Free/static function task (no object pointer)
  template <bool M = kIsMember, typename std::enable_if_t<!M, int> = 0>
  SBJTask(const char*  name,
          uint32_t     stackDepth,
          TaskPriority priority = TaskPriority::Low,
          CoreID       coreId   = 0)
#if SBJVTask
  : _obj(nullptr)
#else
  : _task(IntervalMs, Iterations, &taskFnWrapper, &sbj_detail::SchedulerSingleton::scheduler, true)
#endif
  {
#if SBJVTask
    (void)xTaskCreatePinnedToCore(
        &espEntry,
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

  // Member method task (requires object pointer)
  template <bool M = kIsMember, typename std::enable_if_t<M, int> = 0>
  SBJTask(const char*  name,
          uint32_t     stackDepth,
          typename sbj_detail::member_class<decltype(Callable)>::type* obj,
          TaskPriority priority = TaskPriority::Low,
          CoreID       coreId   = 0)
#if SBJVTask
  : _obj(static_cast<void*>(obj))
#else
  : _task(IntervalMs, Iterations, &taskMemWrapper, &sbj_detail::SchedulerSingleton::scheduler, true)
#endif
  {
#if SBJVTask
    (void)xTaskCreatePinnedToCore(
        &espEntry,
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

  // Must be non-copyable/non-movable: task captures `this` (ESP32) / scheduler owns callback (non-ESP32)
  SBJTask(const SBJTask&) = delete;
  SBJTask& operator=(const SBJTask&) = delete;
  SBJTask(SBJTask&&) = delete;
  SBJTask& operator=(SBJTask&&) = delete;

private:
#if SBJVTask
  // For member-method tasks: stores object pointer. For free function tasks: nullptr.
  void* const _obj;

  // Unified invoke (inlined & optimized via if constexpr)
  inline void invoke() {
    if constexpr (!kIsMember) {
      // Callable is void(*)()
      Callable();
    } else {
      using C = typename sbj_detail::member_class<decltype(Callable)>::type;
      auto* obj = static_cast<C*>(_obj);
      if (obj) {
        (obj->*Callable)();
      }
    }
  }

  // Fully collapsed entry point for all call kinds
  static void espEntry(void* pv) {
    auto* self = static_cast<SBJTask*>(pv);
    if (!self) { vTaskDelete(nullptr); }

    if (startDelayTicks == 0) taskYIELD();
    else vTaskDelay(startDelayTicks);

    if constexpr (Iterations == FOREVER) {
      for (;;) {
        self->invoke();
        if (intervalTicks == 0) taskYIELD();
        else vTaskDelay(intervalTicks);
      }
    } else {
      for (int32_t i = 0; i < Iterations; ++i) {
        self->invoke();
        if (i + 1 < Iterations) {
          if (intervalTicks == 0) taskYIELD();
          else vTaskDelay(intervalTicks);
        }
      }
    }

    vTaskDelete(nullptr);
  }

#else
  // Non-ESP: each SBJTask owns its TaskScheduler::Task
  Task _task;

  // Wrapper for free/static function
  static inline void taskFnWrapper() {
    Callable();
  }

  #define _TASK_SCHEDULER_GETCURRENTTASK

  // Wrapper for member method (object is stored in the task's LTS pointer)
static inline void taskMemWrapper() {
  Task* cur = nullptr;

  // Prefer Scheduler API when present
  #if defined(_TASK_SCHEDULER_GETCURRENTTASK) || defined(TASK_SCHEDULER_HAS_GETCURRENTTASK)
    cur = sbj_detail::SchedulerSingleton::scheduler.getCurrentTask();
  #else
    // Fall back to static API when present (arkhipenko/TaskScheduler)
    cur = Task::getCurrentTask();
  #endif

  using C = typename sbj_detail::member_class<decltype(Callable)>::type;
  auto* obj = cur ? static_cast<C*>(cur->getLtsPointer()) : nullptr;
  if (!obj) return;
  (obj->*Callable)();
}
#endif
};
