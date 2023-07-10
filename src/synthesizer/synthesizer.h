//
// Created by Tim Holzhey on 14.06.23
//

#ifndef FM_SYNTHESIZER_SYNTHESIZER_H
#define FM_SYNTHESIZER_SYNTHESIZER_H

#include "common.h"
#include "portaudio.h"
#include "patch_file.h"
#include "config.h"

typedef enum {
	ENVELOPE_STATE_ATTACK,
	ENVELOPE_STATE_DECAY,
	ENVELOPE_STATE_SUSTAIN,
	ENVELOPE_STATE_RELEASE,
	ENVELOPE_STATE_OFF,
} envelope_state_t;

typedef struct {
	envelope_state_t state;
	int32_t level;
} envelope_data_t;

typedef struct {
	uint32_t phase;
	int32_t input_mod_buffer;
	envelope_data_t envelope_data;
} operator_data_t;

typedef struct {
	uint8_t enable:1;
	uint8_t gate:1;
	uint8_t note;
	int32_t feedback_buffer;
	operator_data_t operator_data[NUM_OPERATORS];
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
