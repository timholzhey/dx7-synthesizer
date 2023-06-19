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
#include "voice.h"

#define SIN_TABLE_SIZE					256

#define INT16_SIGN_BIT					0x8000
#define INT_BIT_WIDTH					24

#define FREQ_TABLE_LOG_NUM_SAMPLES		10
#define FREQ_TABLE_NUM_SAMPLES			(1 << FREQ_TABLE_LOG_NUM_SAMPLES)
#define FREQ_TABLE_SAMPLE_SHIFT			(INT_BIT_WIDTH - FREQ_TABLE_LOG_NUM_SAMPLES)
#define FREQ_TABLE_MAX_INT				20

#define ALGORITHM_ROUTING_TABLE_SIZE	31

#define OSCILLATOR_MODE_RATIO			0
#define OSCILLATOR_MODE_FIXED			1

#define MIDI_NUM_KEYS					128

#define OUTPUT_MODE_INDEX_1			(1 << 0)
#define OUTPUT_MODE_INDEX_2			(1 << 1)
#define OUTPUT_MODE_INDEX_3			(1 << 2)
#define OUTPUT_MODE_INDEX_4			(1 << 3)
#define OUTPUT_MODE_INDEX_5			(1 << 4)
#define OUTPUT_MODE_INDEX_6			(1 << 5)
#define OUTPUT_MOD_INDEX_MASTER		(1 << 6)

// Dynamically populated tables
static uint32_t note_to_log_freq_table[MIDI_NUM_KEYS];
static int32_t log_freq_to_phase_table[FREQ_TABLE_NUM_SAMPLES + 1];

// Precomputed tables
static const int32_t log_freq_coarse_mult_table[] = {
		-16777216, 0, 16777216, 26591258, 33554432, 38955489, 43368474, 47099600,
		50331648, 53182516, 55732705, 58039632, 60145690, 62083076, 63876816,
		65546747, 67108864, 68576247, 69959732, 71268397, 72509921, 73690858,
		74816848, 75892776, 76922906, 77910978, 78860292, 79773775, 80654032,
		81503396, 82323963, 83117622
};
static const uint16_t sin_log_table[SIN_TABLE_SIZE] = {
		2137, 1731, 1543, 1419, 1326, 1252, 1190, 1137, 1091, 1050, 1013, 979, 949, 920, 894, 869,
		846, 825, 804, 785, 767, 749, 732, 717, 701, 687, 672, 659, 646, 633, 621, 609,
		598, 587, 576, 566, 556, 546, 536, 527, 518, 509, 501, 492, 484, 476, 468, 461,
		453, 446, 439, 432, 425, 418, 411, 405, 399, 392, 386, 380, 375, 369, 363, 358,
		352, 347, 341, 336, 331, 326, 321, 316, 311, 307, 302, 297, 293, 289, 284, 280,
		276, 271, 267, 263, 259, 255, 251, 248, 244, 240, 236, 233, 229, 226, 222, 219,
		215, 212, 209, 205, 202, 199, 196, 193, 190, 187, 184, 181, 178, 175, 172, 169,
		167, 164, 161, 159, 156, 153, 151, 148, 146, 143, 141, 138, 136, 134, 131, 129,
		127, 125, 122, 120, 118, 116, 114, 112, 110, 108, 106, 104, 102, 100, 98, 96,
		94, 92, 91, 89, 87, 85, 83, 82, 80, 78, 77, 75, 74, 72, 70, 69,
		67, 66, 64, 63, 62, 60, 59, 57, 56, 55, 53, 52, 51, 49, 48, 47,
		46, 45, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30,
		29, 28, 27, 26, 25, 24, 23, 23, 22, 21, 20, 20, 19, 18, 17, 17,
		16, 15, 15, 14, 13, 13, 12, 12, 11, 10, 10, 9, 9, 8, 8, 7,
		7, 7, 6, 6, 5, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2,
		2, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0
};
static const uint16_t sin_exp_table[SIN_TABLE_SIZE] = {
		0, 3, 6, 8, 11, 14, 17, 20, 22, 25, 28, 31, 34, 37, 40, 42,
		45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81, 84, 87, 90,
		93, 96, 99, 102, 105, 108, 111, 114, 117, 120, 123, 126, 130, 133, 136, 139,
		142, 145, 148, 152, 155, 158, 161, 164, 168, 171, 174, 177, 181, 184, 187, 190,
		194, 197, 200, 204, 207, 210, 214, 217, 220, 224, 227, 231, 234, 237, 241, 244,
		248, 251, 255, 258, 262, 265, 268, 272, 276, 279, 283, 286, 290, 293, 297, 300,
		304, 308, 311, 315, 318, 322, 326, 329, 333, 337, 340, 344, 348, 352, 355, 359,
		363, 367, 370, 374, 378, 382, 385, 389, 393, 397, 401, 405, 409, 412, 416, 420,
		424, 428, 432, 436, 440, 444, 448, 452, 456, 460, 464, 468, 472, 476, 480, 484,
		488, 492, 496, 501, 505, 509, 513, 517, 521, 526, 530, 534, 538, 542, 547, 551,
		555, 560, 564, 568, 572, 577, 581, 585, 590, 594, 599, 603, 607, 612, 616, 621,
		625, 630, 634, 639, 643, 648, 652, 657, 661, 666, 670, 675, 680, 684, 689, 693,
		698, 703, 708, 712, 717, 722, 726, 731, 736, 741, 745, 750, 755, 760, 765, 770,
		774, 779, 784, 789, 794, 799, 804, 809, 814, 819, 824, 829, 834, 839, 844, 849,
		854, 859, 864, 869, 874, 880, 885, 890, 895, 900, 906, 911, 916, 921, 927, 932,
		937, 942, 948, 953, 959, 964, 969, 975, 980, 986, 991, 996, 1002, 1007, 1013, 1018
};
static const uint8_t algorithm_routing_table[ALGORITHM_ROUTING_TABLE_SIZE][NUM_OPERATORS] = {
		{OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5 | OUTPUT_MODE_INDEX_6},
		{OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1 | OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5},
		{OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5 | OUTPUT_MODE_INDEX_6},
		{OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER | OUTPUT_MODE_INDEX_6, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5},
		{OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_5 | OUTPUT_MODE_INDEX_6},
		// TODO
};

synth_data_t synth_data;

ret_code_t synthesizer_init(void) {
	// Init note to log frequency table
	uint32_t base = 50857777;  // (1 << 24) * (log(440) / log(2) - 69/12)
	uint32_t step = (1 << INT_BIT_WIDTH) / 12;
	for (uint8_t i = 0; i < MIDI_NUM_KEYS; i++) {
		note_to_log_freq_table[i] = base + step * i;
	}

	// Init log frequency to frequency table
	double y = (double) (1LL << (INT_BIT_WIDTH + FREQ_TABLE_MAX_INT)) / AUDIO_SAMPLE_RATE;
	double inc = pow(2, 1.0 / FREQ_TABLE_NUM_SAMPLES);
	for (int i = 0; i < FREQ_TABLE_NUM_SAMPLES + 1; i++) {
		log_freq_to_phase_table[i] = (int32_t) floor(y + 0.5);
		y *= inc;
	}

	// Load default patch file
	RET_ON_FAIL(patch_file_load_rom(DEFAULT_PATCH_FILE));

	// Load default sample
	RET_ON_FAIL(patch_file_load_patch(DEFAULT_PATCH_FILE_VOICE - 1, &synth_data.voice_params));

	// Test 1 operator
	voice_params_t params = {
			.algorithm = 1,
			.name = "TEST      ",
			.operators = {
					[0] = {
						.osc = {.frequency_coarse = 1},
						.output_level = 99,
					}
			}
	};
	memcpy(&synth_data.voice_params, &params, sizeof(voice_params_t));
//	voice_assign_key(60, 127);

	return RET_CODE_OK;
}

int32_t get_phase_from_log_frequency(int32_t log_freq) {
	uint8_t index = (log_freq & 0xFFFFFF) >> FREQ_TABLE_SAMPLE_SHIFT;
	int32_t freq_low = log_freq_to_phase_table[index];
	int32_t freq_high = log_freq_to_phase_table[index + 1];
	int32_t low_bits = log_freq & ((1 << FREQ_TABLE_SAMPLE_SHIFT) - 1);
	int32_t high_bits = log_freq >> INT_BIT_WIDTH;
	int32_t freq = freq_low + (int32_t) ((((int64_t) (freq_high - freq_low) * (int64_t) low_bits)) >> FREQ_TABLE_SAMPLE_SHIFT);
	return freq >> (FREQ_TABLE_MAX_INT - high_bits);
}

int32_t get_oscillator_log_frequency(uint8_t midi_note, uint8_t mode, uint8_t coarse, uint8_t fine, uint8_t detune) {
	if (mode == OSCILLATOR_MODE_RATIO) {
		int32_t log_freq = (int32_t) note_to_log_freq_table[midi_note];

		double detune_ratio = 0.0209 * exp(-0.396 * (((float)log_freq) / (1 << INT_BIT_WIDTH))) / 7;
		log_freq += (int32_t) detune_ratio * log_freq * (detune - 7);

		log_freq += log_freq_coarse_mult_table[coarse & 31];
		if (fine) {
			// (1 << 24) / log(2)
			log_freq += (int32_t)floor(24204406.323123 * log(1 + 0.01 * fine) + 0.5);
		}

		return log_freq;
	}

	int32_t log_freq = (4458616 * ((coarse & 3) * 100 + fine)) >> 3;
	log_freq += detune > 7 ? 13457 * (detune - 7) : 0;
	return log_freq;
}

uint16_t get_log_sin_from_angle(uint16_t phi) {
	const uint8_t index = (phi & 0xff);

	switch ((phi & 0x0300)) {
		case 0x0000:
			// rising quarter wave  Shape A
			return sin_log_table[index];
		case 0x0100:
			// falling quarter wave  Shape B
			return sin_log_table[index ^ 0xFF];
		case 0x0200:
			// rising quarter wave -ve  Shape C
			return sin_log_table[index] | INT16_SIGN_BIT;
		default:
			// falling quarter wave -ve  Shape D
			return sin_log_table[index ^ 0xFF] | INT16_SIGN_BIT;
	}
}

// Envelope from 0 (loud) to 512 (silent)
int16_t get_sin_from_angle(uint16_t phase, uint16_t envelope) {
	uint16_t exp_val = get_log_sin_from_angle(phase) + (envelope << 3);
	bool is_signed = exp_val & INT16_SIGN_BIT;
	exp_val &= ~INT16_SIGN_BIT;
	uint32_t result = 0x0400 + sin_exp_table[(exp_val & 0xFF) ^ 0xFF];
	result <<= 1;
	result >>= (exp_val >> 8);

	if (is_signed) {
		return -result-1;
	}
	return result;
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
			if (data->voice_data[voice_idx].gate == 0) {
				continue;
			}

			// Clear input buffers
			for (uint32_t k = 0; k < NUM_OPERATORS; k++) {
				operator_data_t *op_data = &data->voice_data[voice_idx].operator_data[k];
				op_data->input_mod_buffer = 0;
				op_data->level_in = 0;
			}

			// Sample operators
			for (uint32_t operator_idx = NUM_OPERATORS - 1; operator_idx < NUM_OPERATORS; operator_idx--) {
				operator_data_t *op_data = &data->voice_data[voice_idx].operator_data[operator_idx];
				operator_params_t *op_params = &data->voice_params.operators[operator_idx];

				// Get frequency
				int32_t log_freq = get_oscillator_log_frequency(data->voice_data[voice_idx].note, op_params->osc.mode, op_params->osc.frequency_coarse, op_params->osc.frequency_fine, op_params->osc.detune);
				int32_t phase = get_phase_from_log_frequency(log_freq);

				uint16_t op_level = 512 - op_params->output_level * 512 / 99;

				// Sample
				int32_t sample = get_sin_from_angle((op_data->phase + op_data->input_mod_buffer) >> 14, op_level) << 14;
				// printf("operator %d, sample %d, phase %d, level %d, log_freq %d\n", operator_idx, sample, op_data->phase, op_data->level_in, log_freq);
				// printf("input buffer %d, phase %d, sample %d\n", op_data->input_mod_buffer, op_data->phase, sample >> 14);

				// Increment phase
				op_data->phase += phase;

				// Route output
				const uint8_t *routing = algorithm_routing_table[data->voice_params.algorithm];
				if (routing[operator_idx] & OUTPUT_MOD_INDEX_MASTER) {
					master_buffer += sample;
				}
				for (uint8_t output_index = 0; output_index < NUM_OPERATORS; output_index++) {
					if (routing[operator_idx] & (1 << output_index)) {
						// printf("routing %d to %d\n", operator_idx, output_index);
						data->voice_data[voice_idx].operator_data[output_index].input_mod_buffer += sample;
					}
				}
			}
		}

		// Add sample to visualization, align with midi frequency
		visualization_add_sample(master_buffer, 0);

		// Write mono to stereo output buffer
		*out++ = master_buffer;
		*out++ = master_buffer;
	}

	return paContinue;
}
