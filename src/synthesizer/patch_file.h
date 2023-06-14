//
// Created by Tim Holzhey on 14.06.23
//

#ifndef FM_SYNTHESIZER_SYX_DECODER_H
#define FM_SYNTHESIZER_SYX_DECODER_H

#include "common.h"
#include <stdint.h>
#include <stdbool.h>

#define PATCH_FILE_NUM_VOICES			32

typedef struct {
	uint8_t rate1;
	uint8_t rate2;
	uint8_t rate3;
	uint8_t rate4;
	uint8_t level1;
	uint8_t level2;
	uint8_t level3;
	uint8_t level4;
} envelope_params_t;

typedef struct {
	uint8_t break_point;
	uint8_t left_depth;
	uint8_t right_depth;
	uint8_t left_curve;
	uint8_t right_curve;
} keyboard_level_scaling_params_t;

typedef struct {
	uint8_t mode;
	uint8_t frequency_coarse;
	uint8_t frequency_fine;
	uint8_t detune;
} oscillator_params_t;

typedef struct {
	envelope_params_t env;
	keyboard_level_scaling_params_t kls;
	oscillator_params_t osc;
	uint8_t keyboard_rate_scaling;
	uint8_t amplitude_modulation_sensitivity;
	uint8_t key_velocity_sensitivity;
	uint8_t output_level;
} operator_params_t;

typedef struct {
	uint8_t speed;
	uint8_t delay;
	uint8_t pitch_modulation_depth;
	uint8_t amplitude_modulation_depth;
	uint8_t sync;
	uint8_t wave;
	uint8_t pitch_modulation_sensitivity;
} lfo_params_t;

typedef struct {
	operator_params_t operators[6];
	envelope_params_t pitch_eg;
	lfo_params_t lfo;
	uint8_t algorithm;
	uint8_t feedback;
	uint8_t oscillator_key_sync;
	uint8_t transpose;
	char name[10];
} voice_params_t;

typedef enum {
	PATCH_FILE_ROM_ROM1A,
	PATCH_FILE_ROM_COUNT,
} patch_file_rom_t;

ret_code_t patch_file_load_rom(patch_file_rom_t patch_file);
ret_code_t patch_file_load_rom_by_name(const char *rom_name);
bool patch_file_is_loaded(void);

patch_file_rom_t patch_file_get_current_rom(void);
uint8_t patch_file_get_current_patch_number(void);

ret_code_t patch_file_load_patch(uint8_t patch_number, voice_params_t *voice_params);
ret_code_t patch_file_load_patch_by_name(const char *patch_name, voice_params_t *voice_params);

const char *patch_file_get_rom_names(void);
voice_params_t *patch_file_get_voice_params(void);

#endif //FM_SYNTHESIZER_SYX_DECODER_H
