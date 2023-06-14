//
// Created by Tim Holzhey on 14.06.23
//

#ifndef FM_SYNTHESIZER_VOICE_H
#define FM_SYNTHESIZER_VOICE_H

#include "common.h"
#include <stdint.h>

ret_code_t voice_assign_key(uint8_t midi_key, uint8_t velocity);

void voice_release_key(uint8_t midi_key, uint8_t velocity);

#endif //FM_SYNTHESIZER_VOICE_H
