#pragma once

#include <Arduino.h>
#include <FS.h>

namespace TheFS
{
  inline fs::FS* _fs = nullptr;
  inline fs::FS& fs() { return *_fs; }
}
