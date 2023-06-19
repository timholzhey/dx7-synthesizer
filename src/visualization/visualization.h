//
// Created by Tim Holzhey on 14.06.23
//

#ifndef FM_SYNTHESIZER_VISUALIZATION_H
#define FM_SYNTHESIZER_VISUALIZATION_H

#include <stdint.h>
#include "common.h"

void visualization_add_sample(int32_t sample, uint32_t align_freq);

ret_code_t visualization_consume_transfer(uint8_t **pp_buffer, uint32_t *p_buffer_size);

#endif //FM_SYNTHESIZER_VISUALIZATION_H
