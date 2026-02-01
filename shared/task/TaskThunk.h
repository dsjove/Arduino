#pragma once

#define _TASK_LTS_POINTER
#include <TaskScheduler.h>

class ScheduledRunner {
public:
  virtual ~ScheduledRunner() = default;
  virtual void loop(Task&) = 0;
};

class TaskThunk {
private:
  inline static Scheduler* s_scheduler;

public:
  TaskThunk(
      Scheduler& scheduler,
      uint32_t intervalMs,
      ScheduledRunner* r,
      bool enabled = true,
      int iterations = TASK_FOREVER)
  : task(intervalMs, iterations, &TaskThunk::callback, &scheduler, enabled)
  , runner(r)
  {
    s_scheduler = &scheduler;
    task.setLtsPointer(this);
  }

private:
  Task task;
  ScheduledRunner* runner;

  static void callback() {
    Task& t = s_scheduler->currentTask();
    auto* self = static_cast<TaskThunk*>(t.getLtsPointer());
    if (!self && !self->runner) return;
    self->runner->loop(t);
  }
};

template<uint8_t N>
class Channel {
public:
  Channel(Scheduler& sched,
          uint32_t intervalMs,
          long iterations = TASK_FOREVER,
          bool startEnabled = true)
  : _task(intervalMs, iterations, &Channel<N>::tick, &sched, startEnabled)
  {}

  void begin() {
    // Nothing required here unless you want extra init
  }

private:
  Task _task;

  // ---- static per-N state ----
  static inline int counter = 0;
  static inline uint32_t lastTick = 0;

  // ---- scheduler callback ----
  static void tick() {
    counter++;
    lastTick = millis();

    // operate on Channel<N>'s static data
  }
};
