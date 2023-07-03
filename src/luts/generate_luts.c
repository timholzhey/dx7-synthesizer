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

static void write_hex_bytes_to_file(uint8_t *p_data, uint32_t num_elements, uint32_t element_size, const char *filename) {
	// build file path
	char file_path[256];
	sprintf(file_path, "%s/%s", LUT_GENERATE_DIR, filename);

	// open file
	FILE *file = fopen(file_path, "w");
	if (file == NULL) {
		return;
	}

	// write comment header
	fprintf(file, "// %s: %d elements [%d bytes each]\n\n", filename, num_elements, element_size);

	// write data
	uint32_t nun_bytes = num_elements * element_size;
	for (uint32_t i = 0; i < nun_bytes; i += element_size) {
		switch (element_size) {
			case 1:
				fprintf(file, "%02x", p_data[i]);
				break;
			case 2:
				fprintf(file, "%02x%02x", p_data[i + 1], p_data[i]);
				break;
			case 4:
				fprintf(file, "%02x%02x%02x%02x", p_data[i + 3], p_data[i + 2], p_data[i + 1], p_data[i]);
				break;
			default:
				log_error("Invalid element size: %d", element_size);
				break;
		}
		if (i % 32 == 32 - element_size) {
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
	write_hex_bytes_to_file(buffer_8, NOTE_TO_LOG_FREQ_TABLE_SIZE, sizeof(uint32_t), "hex_u32_note_to_log_freq.mem");


	// Log frequency to phase table
	double log_freq_to_phase_base = (1LL << (SAMPLE_BIT_WIDTH + LOG_FREQ_TO_PHASE_TABLE_BIT_WIDTH)) / AUDIO_SAMPLE_RATE;
	double log_freq_to_phase_mult = pow(2, 1.0 / LOG_FREQ_TO_PHASE_TABLE_SIZE);
	for (uint32_t i = 0; i < LOG_FREQ_TO_PHASE_TABLE_SIZE + 1; i++) {
		buffer_32[i] = (uint32_t) floor(log_freq_to_phase_base + 0.5);
		log_freq_to_phase_base *= log_freq_to_phase_mult;
	}
	write_hex_bytes_to_file(buffer_8, LOG_FREQ_TO_PHASE_TABLE_SIZE + 1, sizeof(uint32_t), "hex_u32_log_freq_to_phase.mem");


	// Log sine table (quarter period)
	for (uint32_t i = 0; i < LOG_SIN_TABLE_SIZE; i++) {
		buffer_16[i] = (uint16_t) round(-(1 << LOG_SIN_TABLE_BIT_WIDTH) * log(sin((i + 0.5) / LOG_SIN_TABLE_SIZE * M_PI_2)) / log(2));
	}
	write_hex_bytes_to_file(buffer_8, LOG_SIN_TABLE_SIZE, sizeof(uint16_t), "hex_u16_log_sin.mem");


	// Exp table
	for (uint32_t i = 0; i < EXP_TABLE_SIZE; i++) {
		buffer_16[i] = (uint16_t) round((1 << EXP_TABLE_BIT_WIDTH) * (pow(2, (double) i / EXP_TABLE_SIZE) - 1));
	}
	write_hex_bytes_to_file(buffer_8, EXP_TABLE_SIZE, sizeof(uint16_t), "hex_u16_exp.mem");


	// Coarse log multiplier table
	for (uint32_t i = 0; i < COARSE_LOG_MULT_TABLE_SIZE; i++) {
		double x = i == 0 ? 0.5 : i;
		buffer_32[i] = (int32_t) ((1 << COARSE_LOG_MULT_TABLE_BIT_WIDTH) * (log(x) / log(2)));
	}
	write_hex_bytes_to_file(buffer_8, COARSE_LOG_MULT_TABLE_SIZE, sizeof(uint32_t), "hex_i32_coarse_log_mult.mem");


	// Fine log multiplier table (like coarse but 1/100th)
	for (uint32_t i = 0; i < FINE_LOG_MULT_TABLE_SIZE; i++) {
		buffer_32[i] = (uint32_t) ((1 << FINE_LOG_MULT_TABLE_BIT_WIDTH) * (log(1 + (double) i / 100) / log(2)));
	}
	write_hex_bytes_to_file(buffer_8, FINE_LOG_MULT_TABLE_SIZE, sizeof(uint32_t), "hex_u32_fine_log_mult.mem");


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
	write_hex_bytes_to_file((uint8_t *) algorithm_routing_table, ALGORITHM_ROUTING_TABLE_SIZE * NUM_OPERATORS, 1, "hex_u8_algorithm_routing.mem");


	// Level scale table
	uint8_t level_scale[LEVEL_SCALE_TABLE_SIZE] = {0, 5, 9, 13, 17, 20, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 42, 43,
												   45, 46, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
												   63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
												   80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96,
												   97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
												   111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,
												   125, 126, 127};
	write_hex_bytes_to_file(level_scale, LEVEL_SCALE_TABLE_SIZE, 1, "hex_u8_level_scale.mem");


	// Level decibel amplitude table

}
