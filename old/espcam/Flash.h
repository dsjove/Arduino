#pragma once

namespace Flash {
    void begin(int pin);

    int getIntensity();

    void setIntensity(int val);

    void setEnabled(bool enabled);

    class On {
    public:
      On(bool enable);
      ~On();
    private:
      const bool _enable;
    };
};
