#pragma once
#include <gpiod.h>


class GpioLink {
public:
    GpioLink(const char* chipPath, unsigned startLine, unsigned dropLine);
    void raiseStart();   // підняти START в 1, тримати (викликати один раз при старті)
    void pulseDrop();    // імпульс DROP ~80мс (викликати один раз, коли влучили)
    ~GpioLink();
private:
    gpiod_chip* chip_;
    gpiod_line_request* request_;
    unsigned startLine_, dropLine_;
};