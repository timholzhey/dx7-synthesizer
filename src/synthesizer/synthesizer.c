//
// Created by Tim Holzhey on 14.06.23
//

#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "synthesizer.h"
#include "patch_file.h"
#include "visualization.h"
#include "config.h"
#include "read_luts.h"

// LUTS
static uint32_t note_to_log_freq_table[NOTE_TO_LOG_FREQ_TABLE_SIZE];
static uint32_t log_freq_to_phase_table[LOG_FREQ_TO_PHASE_TABLE_SIZE + 1];
static uint16_t log_sin_table[LOG_SIN_TABLE_SIZE];
static uint16_t exp_table[EXP_TABLE_SIZE];
static int32_t coarse_log_mult_table[COARSE_LOG_MULT_TABLE_SIZE];
static uint32_t fine_log_mult_table[FINE_LOG_MULT_TABLE_SIZE];
static uint8_t algorithm_routing_table[ALGORITHM_ROUTING_TABLE_SIZE][NUM_OPERATORS];
static uint8_t level_scale_table[LEVEL_SCALE_TABLE_SIZE];

static int32_t get_sin_from_angle(uint32_t phase, uint16_t level);
static uint16_t get_log_sin_from_angle(uint16_t phi);
static uint32_t get_oscillator_log_frequency(uint8_t midi_note, uint8_t mode, uint8_t coarse, uint8_t fine, uint8_t detune);
static uint32_t get_phase_from_log_frequency(uint32_t log_freq);
static uint32_t envelope_get_sample(uint8_t gate, uint8_t output_level, envelope_params_t *p_env_params, envelope_data_t *p_env_data);

synth_data_t synth_data;

ret_code_t synthesizer_init(void) {
	RET_ON_FAIL(READ_LUT("hex_u32_note_to_log_freq.mem", note_to_log_freq_table));
	RET_ON_FAIL(READ_LUT("hex_u32_log_freq_to_phase.mem", log_freq_to_phase_table));
	RET_ON_FAIL(READ_LUT("hex_u16_log_sin.mem", log_sin_table));
	RET_ON_FAIL(READ_LUT("hex_u16_exp.mem", exp_table));
	RET_ON_FAIL(READ_LUT("hex_i32_coarse_log_mult.mem", coarse_log_mult_table));
	RET_ON_FAIL(READ_LUT("hex_u32_fine_log_mult.mem", fine_log_mult_table));
	RET_ON_FAIL(READ_LUT("hex_u8_level_scale.mem", level_scale_table));
	uint64_t algorithm_routing_table_raw[ALGORITHM_ROUTING_TABLE_SIZE];
	RET_ON_FAIL(READ_LUT("hex_u64_algorithm_routing.mem", algorithm_routing_table_raw));
	for (uint32_t i = 0; i < ALGORITHM_ROUTING_TABLE_SIZE; i++) {
		for (uint32_t j = 0; j < NUM_OPERATORS; j++) {
			algorithm_routing_table[i][j] = (algorithm_routing_table_raw[i] >> (j * 8)) & 0xFF;
		}
	}

	// Load default patch file
	RET_ON_FAIL(patch_file_load_rom(DEFAULT_PATCH_FILE));

	// Load default sample
	RET_ON_FAIL(patch_file_load_patch(DEFAULT_PATCH_FILE_VOICE - 1, &synth_data.voice_params));

	return RET_CODE_OK;
}

/**
 * @brief Get the phase increment for a given log2(frequency) value. Linearly interpolates between the two closest values in the table.
 *
 * @param log_freq
 * @return phase increment
 */
static uint32_t get_phase_from_log_frequency(uint32_t log_freq) {
	uint16_t index = (log_freq & SAMPLE_MASK) >> LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT;
	uint32_t phase_low = log_freq_to_phase_table[index];
	uint32_t phase_high = log_freq_to_phase_table[index + 1];
	uint32_t low_bits = log_freq & ((1 << LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT) - 1);
	uint32_t high_bits = log_freq >> SAMPLE_BIT_WIDTH;
	uint64_t slope = (int64_t) (phase_high - phase_low) * (int64_t) low_bits;
	uint32_t phase = phase_low + (int32_t) (slope >> LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT);
	return LOG_FREQ_TO_PHASE_TABLE_BIT_WIDTH >= high_bits ? phase >> (LOG_FREQ_TO_PHASE_TABLE_BIT_WIDTH - high_bits) : 0;
}

/**
 * @brief Get the oscillator log2(frequency) value for a given MIDI note and oscillator parameters.
 *
 * @param midi_note 0..127
 * @param mode 0 = ratio, 1 = fixed
 * @param coarse 0..31
 * @param fine 0..99 (multiplier 1.00..1.99)
 * @param detune 0..14 (7 = no detune)
 * @return log frequency
 */
static uint32_t get_oscillator_log_frequency(uint8_t midi_note, uint8_t mode, uint8_t coarse, uint8_t fine, uint8_t detune) {
	if (mode == OSCILLATOR_MODE_RATIO) {
		uint32_t log_freq = note_to_log_freq_table[midi_note];
		log_freq += (detune - 7) << LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT;
		log_freq += coarse_log_mult_table[coarse % COARSE_LOG_MULT_TABLE_SIZE];
		log_freq += fine_log_mult_table[fine % FINE_LOG_MULT_TABLE_SIZE];
		return log_freq;
	}

	// ((1 << 24) * log(10) / log(2) * .01) << 3
	uint32_t log_freq = (4458616 * ((coarse & 3) * 100 + fine)) >> 3;
	log_freq += (detune - 7) << LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT;
	return log_freq;
}

/**
 * @brief Get the log2(sin(phi)) value for a given angle.
 *
 * @param phi Angle
 * @return log sin value
 */
static uint16_t get_log_sin_from_angle(uint16_t phi) {
	uint8_t index = phi & LOG_SIN_TABLE_MASK;
	uint8_t quadrant = (phi >> LOG_SIN_TABLE_BIT_WIDTH) & 3;

	switch (quadrant) {
		case 0:
			return log_sin_table[index];
		case 1:
			return log_sin_table[index ^ (LOG_SIN_TABLE_SIZE - 1)];
		case 2:
			return log_sin_table[index] | INT16_SIGN_BIT;
		default:
			return log_sin_table[index ^ (LOG_SIN_TABLE_SIZE - 1)] | INT16_SIGN_BIT;
	}
}

/**
 * @brief Get the sin(phi) value for a given angle and level.
 *
 * @param phase
 * @param level 0..ENVELOPE_MAX (loud to quiet)
 * @return sin value
 */
static int32_t get_sin_from_angle(uint32_t phase, uint16_t level) {
	// log2(sin(phi)) + level
	uint16_t log_sin = get_log_sin_from_angle(phase >> LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT) + (level << (ENVELOPE_BIT_WIDTH - 6));

	bool is_signed = log_sin & INT16_SIGN_BIT;
	log_sin &= ~INT16_SIGN_BIT;

	// 2^(log2(sin(phi)) + level)
	uint32_t result = (1 << EXP_TABLE_BIT_WIDTH) + exp_table[(log_sin & (EXP_TABLE_SIZE - 1)) ^ (EXP_TABLE_SIZE - 1)];
	result <<= 1;
	result >>= (log_sin >> EXP_TABLE_LOG_SIZE);

	return is_signed ? -result << LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT : result << LOG_FREQ_TO_PHASE_TABLE_SAMPLE_SHIFT;
}

/**
 * @brief Advances the envelope state machine and returns the next level sample.
 *
 * @param gate 0 = off, 1 = on
 * @param output_level 0..99 (quiet to loud)
 * @param p_env_params
 * @param p_env_data
 * @return level 0..ENVELOPE_MAX (loud to quiet)
 */
static uint32_t envelope_get_sample(uint8_t gate, uint8_t output_level, envelope_params_t *p_env_params, envelope_data_t *p_env_data) {
	if (p_env_data->state != ENVELOPE_STATE_OFF && gate == 0) {
		p_env_data->state = ENVELOPE_STATE_RELEASE;
	}

	if (p_env_data->state < ENVELOPE_STATE_RELEASE || (p_env_data->state == ENVELOPE_STATE_RELEASE && gate == 0)) {
		int32_t target_level = level_scale_table[p_env_params->levels[p_env_data->state]] << 24;
		uint32_t delta_level = ((p_env_params->rates[p_env_data->state] * ((p_env_params->rates[p_env_data->state] >> 2) + 4)) + 16) << 9;

		if (target_level > p_env_data->level) {
			p_env_data->level += delta_level;
			if (p_env_data->level > target_level) {
				p_env_data->level = target_level;
				p_env_data->state++;
			}
		} else {
			p_env_data->level -= (delta_level >> 11) * (delta_level >> 11);
			if (p_env_data->level < target_level) {
				p_env_data->level = target_level;
				p_env_data->state++;
			}
		}
	}

	return ((((p_env_data->level >> 24) + 1) * (level_scale_table[output_level] + 1)) >> 5) - 1;
}

int synthesizer_render(const void *input_buffer, void *output_buffer,
					   unsigned long frames_per_buffer,
					   const PaStreamCallbackTimeInfo *time_info,
					   PaStreamCallbackFlags status_flags,
					   void *user_data) {
	synth_data_t *data = (synth_data_t *) user_data;
	int32_t *out = (int32_t *) output_buffer;

	(void) time_info;
	(void) status_flags;
	(void) input_buffer;

	for (uint32_t frame_idx = 0; frame_idx < frames_per_buffer; frame_idx++) {
		int32_t master_buffer = 0;

		for (uint32_t voice_idx = 0; voice_idx < NUM_VOICES; voice_idx++) {
			if (data->voice_data[voice_idx].enable == 0) {
				continue;
			}

			const uint8_t *routing = algorithm_routing_table[data->voice_params.algorithm];

			// Init input buffers
			for (uint32_t operator_idx = 0; operator_idx < NUM_OPERATORS; operator_idx++) {
				operator_data_t *op_data = &data->voice_data[voice_idx].operator_data[operator_idx];
				op_data->input_mod_buffer = 0;
				if (routing[operator_idx] & (1 << operator_idx)) {
					op_data->input_mod_buffer = data->voice_data[voice_idx].feedback_buffer >> (FEEDBACK_BIT_WIDTH - data->voice_params.feedback + 1);
				}
			}

			// Sample operators
			for (int32_t operator_idx = NUM_OPERATORS - 1; operator_idx >= 0; operator_idx--) {
				operator_data_t *op_data = &data->voice_data[voice_idx].operator_data[operator_idx];
				operator_params_t *op_params = &data->voice_params.operators[operator_idx];

				// Get frequency and phase increment
				uint32_t log_freq = get_oscillator_log_frequency(data->voice_data[voice_idx].note, op_params->osc.mode, op_params->osc.frequency_coarse, op_params->osc.frequency_fine, op_params->osc.detune);
				uint32_t phase_inc = get_phase_from_log_frequency(log_freq);

				// Get level
				uint16_t op_level = ENVELOPE_MAX - envelope_get_sample(data->voice_data[voice_idx].gate,
																	   op_params->output_level,
																	   &op_params->env, &op_data->envelope_data);

				// Sample sine wave
				int32_t sample = get_sin_from_angle(op_data->phase + op_data->input_mod_buffer, op_level);

				// Increment phase
				op_data->phase += phase_inc;

				// Route to master buffer
				if (routing[operator_idx] & OUTPUT_MOD_INDEX_MASTER) {
					master_buffer += sample;
				}

				// Route to feedback buffer
				if (routing[operator_idx] & (1 << operator_idx)) {
					data->voice_data[voice_idx].feedback_buffer = sample;
				}

				// Route to other operators
				for (uint8_t output_index = 0; output_index < NUM_OPERATORS; output_index++) {
					if (routing[operator_idx] & (1 << output_index)) {
						data->voice_data[voice_idx].operator_data[output_index].input_mod_buffer += (sample * 100) >> 7;
					}
				}
			}
		}

		// Find lower midi note playing
		uint8_t lowest_note = 127;
		for (uint32_t voice_idx = 0; voice_idx < NUM_VOICES; voice_idx++) {
			if (data->voice_data[voice_idx].gate == 0) {
				continue;
			}
			if (data->voice_data[voice_idx].note < lowest_note) {
				lowest_note = data->voice_data[voice_idx].note;
			}
		}

		// Add sample to visualization, align with midi frequency
		visualization_add_sample(master_buffer, lowest_note);

		// Amplify
		master_buffer <<= 1;

		// Write mono to stereo output buffer
		*out++ = master_buffer;
		*out++ = master_buffer;
	}

	return paContinue;
}
