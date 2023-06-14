//
// Created by Tim Holzhey on 14.06.23
//

#ifndef FM_SYNTHESIZER_VISUALIZATION_H
#define FM_SYNTHESIZER_VISUALIZATION_H

#include <stdint.h>
#include "common.h"

void visualization_route_websocket_stream(void);

void visualization_add_sample(int32_t sample, uint32_t align_freq);

#endif //FM_SYNTHESIZER_VISUALIZATION_H
