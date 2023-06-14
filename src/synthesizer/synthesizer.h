//
// Created by Tim Holzhey on 14.06.23
//

#ifndef FM_SYNTHESIZER_SYNTHESIZER_H
#define FM_SYNTHESIZER_SYNTHESIZER_H

#include "common.h"
#include "portaudio.h"
#include "patch_file.h"

#define NUM_VOICES					16
#define NUM_OPERATORS_PER_VOICE		6

typedef struct {
	int32_t phase;
	int32_t input_mod_buffer;
	int32_t level_in;
	int32_t level_out;
} operator_data_t;

typedef struct {
	uint8_t gate:1;
	uint8_t note;
	operator_data_t operator_data[NUM_OPERATORS_PER_VOICE];
} voice_data_t;

typedef struct {
	voice_data_t voice_data[NUM_VOICES];
	voice_params_t voice_params;
} synth_data_t;

extern synth_data_t synth_data;

ret_code_t synthesizer_init(void);

int synthesizer_render(const void *input_buffer, void *output_buffer,
					   unsigned long frames_per_buffer,
					   const PaStreamCallbackTimeInfo *time_info,
					   PaStreamCallbackFlags status_flags,
					   void *user_data);

#endif //FM_SYNTHESIZER_SYNTHESIZER_H
