//
// Created by Tim Holzhey on 25.06.23.
//

#include <stdlib.h>
#include "read_luts.h"

static uint8_t hex_to_byte(char *p_data) {
	uint8_t byte = 0;
	for (uint8_t i = 0; i < 2; i++) {
		byte <<= 4;
		if (p_data[i] >= '0' && p_data[i] <= '9') {
			byte |= p_data[i] - '0';
		} else if (p_data[i] >= 'A' && p_data[i] <= 'F') {
			byte |= p_data[i] - 'A' + 10;
		} else if (p_data[i] >= 'a' && p_data[i] <= 'f') {
			byte |= p_data[i] - 'a' + 10;
		} else {
			log_error("Invalid hex byte: %c%c", p_data[i], p_data[i + 1]);
			return 0;
		}
	}
	return byte;
}

ret_code_t read_lut(const char *filename, uint8_t *p_data, uint32_t num_elements, uint32_t element_size) {
	// build file path
	char path[256];
	snprintf(path, sizeof(path), "%s/%s", LUT_GENERATE_DIR, filename);

	// open file
	FILE *file = fopen(path, "r");
	if (file == NULL) {
		log_error("Failed to open file %s.", path);
		return RET_CODE_ERROR;
	}

	// get file size
	fseek(file, 0, SEEK_END);
	uint32_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	// read file into buffer
	char *buffer = malloc(file_size + 1);
	if (buffer == NULL) {
		log_error("Failed to allocate memory for file %s.", path);
		fclose(file);
		return RET_CODE_ERROR;
	}
	if (fread(buffer, 1, file_size, file) != file_size) {
		log_error("Failed to read file %s.", path);
		fclose(file);
		free(buffer);
		return RET_CODE_ERROR;
	}

	// parse file: // comments, ignore whitespace, hex bytes
	uint32_t buf_in_idx = 0;
	uint32_t buf_out_idx = 0;
	uint32_t element_index = 0;
	while (buf_in_idx < file_size && element_index < num_elements) {
		// Skip line if // comment
		if (buffer[buf_in_idx] == '/' && buffer[buf_in_idx + 1] == '/') {
			while (buffer[buf_in_idx] != '\n' && buf_in_idx < file_size) {
				buf_in_idx++;
			}
			continue;
		}
		// Ignore whitespace
		if (buffer[buf_in_idx] == ' ' || buffer[buf_in_idx] == '\n' || buffer[buf_in_idx] == '\r' || buffer[buf_in_idx] == '\t') {
			buf_in_idx++;
			continue;
		}
		// Try parse hex byte with word size e.g. ABCD E123 4567 89AB
		uint8_t word[4];
		switch (element_size) {
			case 1:
				word[0] = hex_to_byte(&buffer[buf_in_idx]);
				break;
			case 2:
				word[1] = hex_to_byte(&buffer[buf_in_idx]);
				word[0] = hex_to_byte(&buffer[buf_in_idx + 2]);
				break;
			case 4:
				word[3] = hex_to_byte(&buffer[buf_in_idx]);
				word[2] = hex_to_byte(&buffer[buf_in_idx + 2]);
				word[1] = hex_to_byte(&buffer[buf_in_idx + 4]);
				word[0] = hex_to_byte(&buffer[buf_in_idx + 6]);
				break;
			default:
				break;
		}

		// Copy word to output buffer
		for (uint32_t i = 0; i < element_size; i++) {
			p_data[buf_out_idx + i] = word[i];
		}
		buf_out_idx += element_size;
		buf_in_idx += element_size * 2;
		element_index++;
	}

	// close file
	fclose(file);

	return RET_CODE_OK;
}
