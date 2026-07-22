#include <unistd.h>
#include <stdio.h>
#include "link/GpioLink.h"

GpioLink::GpioLink(const char* chipPath, unsigned startLine, unsigned dropLine)
    : chip_(nullptr), request_(nullptr), startLine_(startLine), dropLine_(dropLine) {
    chip_ = gpiod_chip_open(chipPath);
    if (!chip_) { perror("gpiod_chip_open"); return ; }

    gpiod_line_settings *settings_output = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(settings_output, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings_output, GPIOD_LINE_VALUE_INACTIVE);

    gpiod_line_config *line_cfg = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(line_cfg, &startLine_, 1, settings_output);
    gpiod_line_config_add_line_settings(line_cfg, &dropLine_, 1, settings_output);

    gpiod_request_config *req_cfg = gpiod_request_config_new();
    gpiod_request_config_set_consumer(req_cfg, "drone_hunter");
    request_ = gpiod_chip_request_lines(chip_, req_cfg, line_cfg);

    // gpiod_chip_request_lines() копіює налаштування собі — тимчасові
    // config-об'єкти більше не потрібні незалежно від результату.
    gpiod_request_config_free(req_cfg);
    gpiod_line_config_free(line_cfg);
    gpiod_line_settings_free(settings_output);

    if (!request_) { perror("gpiod_chip_request_lines"); return ; }
}

void GpioLink::raiseStart() {
    if (!request_) return;  // чип/лінії не вдалося отримати при конструюванні
    gpiod_line_request_set_value(request_, startLine_, GPIOD_LINE_VALUE_ACTIVE);
}

void GpioLink::pulseDrop() {
    if (!request_) return;
    gpiod_line_request_set_value(request_, dropLine_, GPIOD_LINE_VALUE_ACTIVE);
    usleep(80000);
    gpiod_line_request_set_value(request_, dropLine_, GPIOD_LINE_VALUE_INACTIVE);
}
GpioLink::~GpioLink() {
    if (request_) gpiod_line_request_release(request_);
    if (chip_) gpiod_chip_close(chip_);
}