#ifndef RedCycle_h
#define RedCycle_h

#include "Arduino.h"

class RedCycle
{
  public:
    void Initialize();
    void Run();
  private:
    int red_count;
    int _rate;
    int _wait;
}

#endif
