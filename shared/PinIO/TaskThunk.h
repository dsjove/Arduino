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
