//
// Created by Tim Holzhey on 24.06.23.
//

#include "generate_luts.h"
#include "config.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#define BUFFER_SIZE			10 * 1024

static uint8_t buffer_8[BUFFER_SIZE];
static uint16_t *buffer_16 = (uint16_t *) buffer_8;
static uint32_t *buffer_32 = (uint32_t *) buffer_8;

static void write_hex_bytes_to_file(uint8_t *p_data, uint32_t data_len, const char *filename) {
	// build file path
	char file_path[256];
	sprintf(file_path, "%s/%s", LUT_GENERATE_DIR, filename);

	// open file
	FILE *file = fopen(file_path, "w");
	if (file == NULL) {
		return;
	}

	// write comment header
	fprintf(file, "// %s [%u bytes]\n\n", filename, data_len);

	// write data
	for (uint32_t i = 0; i < data_len; i++) {
		fprintf(file, "%02x", p_data[i]);
		if (i % 16 == 15) {
			fprintf(file, "\n");
		} else {
			fprintf(file, " ");
		}
	}

	// close file
	fclose(file);
}

int main(void) {
	// Note (MIDI) to log frequency table
	uint32_t note_to_log_freq_base = (uint32_t) ((1 << SAMPLE_BIT_WIDTH) * (log(BASE_PITCH_FREQUENCY_HZ) / log(2) - (double) BASE_PITCH_MIDI_NOTE / HALF_TONES_PER_OCTAVE));
	uint32_t note_to_log_freq_step = (1 << SAMPLE_BIT_WIDTH) / HALF_TONES_PER_OCTAVE;
	for (uint32_t i = 0; i < NOTE_TO_LOG_FREQ_TABLE_SIZE; i++) {
		buffer_32[i] = note_to_log_freq_base + note_to_log_freq_step * i;
	}
	write_hex_bytes_to_file(buffer_8, NOTE_TO_LOG_FREQ_TABLE_SIZE * sizeof(uint32_t), "hex_u32_note_to_log_freq.mem");


	// Log frequency to phase table
	double log_freq_to_phase_base = (1LL << (SAMPLE_BIT_WIDTH + LOG_FREQ_TO_PHASE_TABLE_BIT_WIDTH)) / AUDIO_SAMPLE_RATE;
	double log_freq_to_phase_mult = pow(2, 1.0 / LOG_FREQ_TO_PHASE_TABLE_SIZE);
	for (uint32_t i = 0; i < LOG_FREQ_TO_PHASE_TABLE_SIZE; i++) {
		buffer_32[i] = (uint32_t) floor(log_freq_to_phase_base + 0.5);
		log_freq_to_phase_base *= log_freq_to_phase_mult;
	}
	write_hex_bytes_to_file(buffer_8, LOG_FREQ_TO_PHASE_TABLE_SIZE * sizeof(uint32_t), "hex_u32_log_freq_to_phase.mem");


	// Log sine table (quarter period)
	for (uint32_t i = 0; i < LOG_SIN_TABLE_SIZE; i++) {
		buffer_16[i] = round(-(1 << LOG_SIN_TABLE_BIT_WIDTH) * log(sin((i + 0.5) / LOG_SIN_TABLE_SIZE * M_PI_2)) / log(2));
	}
	write_hex_bytes_to_file(buffer_8, LOG_SIN_TABLE_SIZE * sizeof(uint16_t), "hex_u16_log_sin.mem");


	// Exp table
	for (uint32_t i = 0; i < EXP_TABLE_SIZE; i++) {
		buffer_16[i] = round((1 << EXP_TABLE_BIT_WIDTH) * (pow(2, (double) i / EXP_TABLE_SIZE) - 1));
	}
	write_hex_bytes_to_file(buffer_8, EXP_TABLE_SIZE * sizeof(uint16_t), "hex_u16_exp.mem");


	// Coarse log multiplier table
	for (uint32_t i = 0; i < COARSE_LOG_MULT_TABLE_SIZE; i++) {
		double x = i == 0 ? 0.5 : i;
		buffer_32[i] = (int32_t) ((1 << COARSE_LOG_MULT_TABLE_BIT_WIDTH) * (log(x) / log(2)));
	}
	write_hex_bytes_to_file(buffer_8, COARSE_LOG_MULT_TABLE_SIZE * sizeof(uint32_t), "hex_i32_coarse_log_mult.mem");


	// Algorithm routing table
	uint8_t algorithm_routing_table[ALGORITHM_ROUTING_TABLE_SIZE][NUM_OPERATORS] = {
			[0] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5 | OUTPUT_MODE_INDEX_6},
			[1] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1 | OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5},
			[2] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5 | OUTPUT_MODE_INDEX_6},
			[3] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER | OUTPUT_MODE_INDEX_6, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_5},
			[4] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_5 | OUTPUT_MODE_INDEX_6},
			[5] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MOD_INDEX_MASTER | OUTPUT_MODE_INDEX_6, OUTPUT_MODE_INDEX_5},
			[6] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_5 | OUTPUT_MODE_INDEX_6},
			[7] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1 | OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_5},
			[8] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MODE_INDEX_2 | OUTPUT_MODE_INDEX_3, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_4},
			[9] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1, OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_4, OUTPUT_MODE_INDEX_4 | OUTPUT_MODE_INDEX_6},
			[10] = {OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_1 | OUTPUT_MODE_INDEX_2, OUTPUT_MOD_INDEX_MASTER, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_3, OUTPUT_MODE_INDEX_3},
			// TODO
	};
	write_hex_bytes_to_file((uint8_t *) algorithm_routing_table, ALGORITHM_ROUTING_TABLE_SIZE * NUM_OPERATORS, "hex_u8_algorithm_routing.mem");
}
