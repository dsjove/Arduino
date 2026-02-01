#pragma once

#include <SD_MMC.h>

class MySDCard {
public:
  MySDCard();

  bool begin();

  operator fs::FS* () { return _fs; }

private:
	fs::FS* _fs;
};
