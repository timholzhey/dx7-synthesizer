//
// Created by Tim Holzhey on 25.06.23.
//

#ifndef FM_SYNTHESIZER_READ_LUTS_H
#define FM_SYNTHESIZER_READ_LUTS_H

#include "config.h"

#define READ_LUT(filename, table) read_lut(filename, (uint8_t *) table, sizeof(table))

ret_code_t read_lut(const char *filename, uint8_t *p_data, uint32_t data_len);

#endif //FM_SYNTHESIZER_READ_LUTS_H
