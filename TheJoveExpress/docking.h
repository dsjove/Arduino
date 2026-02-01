#pragma once

#include <Arduino.h>
#include <TaskScheduler.h>

#include "src/PinIO/Mcp23017PinIO.h"

namespace docking
{
  inline constexpr PinIO<15, GpioMode::DigitalIn, Mcp23017PinIO<>> DockDetect{}; // GPB7
  
  // Cached raw level + convenience bool
  inline volatile GpioLevel dockLevel = GpioLevel::Low;
  inline volatile bool isDocked  = false;

  inline void _tick()
  {
    const auto v = DockDetect.read();
    dockLevel = v;
    isDocked  = (v == GpioLevel::High);
  }

  inline Task task(500, TASK_FOREVER, &_tick);

  inline void begin(Scheduler& sched)
  {
    DockDetect.begin();
    sched.addTask(task);
    task.enable();
  }
}
