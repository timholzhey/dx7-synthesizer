//
// Created by Tim Holzhey on 25.06.23.
//

#include <stdlib.h>
#include "read_luts.h"

ret_code_t read_lut(const char *filename, uint8_t *p_data, uint32_t data_len) {
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
	uint32_t buffer_index = 0;
	uint32_t data_index = 0;
	while (buffer_index < file_size && data_index < data_len) {
		// skip whitespace
		while (buffer_index < file_size && (buffer[buffer_index] == ' ' || buffer[buffer_index] == '\n' || buffer[buffer_index] == '\r')) {
			buffer_index++;
		}
		// skip comments
		if (buffer_index < file_size && buffer[buffer_index] == '/') {
			buffer_index++;
			if (buffer_index < file_size && buffer[buffer_index] == '/') {
				buffer_index++;
				while (buffer_index < file_size && buffer[buffer_index] != '\n') {
					buffer_index++;
				}
			}
		}
		// parse hex byte
		if (buffer_index < file_size && buffer[buffer_index] != ' ' && buffer[buffer_index] != '\n' && buffer[buffer_index] != '\r') {
			char hex_byte[3];
			hex_byte[0] = buffer[buffer_index++];
			hex_byte[1] = buffer[buffer_index++];
			hex_byte[2] = '\0';
			p_data[data_index++] = (uint8_t) strtol(hex_byte, NULL, 16);
		}
	}

	// close file
	fclose(file);

	return RET_CODE_OK;
}
