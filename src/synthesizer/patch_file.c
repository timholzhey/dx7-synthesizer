//
// Created by Tim Holzhey on 14.06.23
//

#include <stdlib.h>
#include "patch_file.h"
#include "common.h"

#define SYSEX_STATUS_START				0xF0
#define SYSEX_STATUS_END				0xF7
#define SYSEX_ID_YAMAHA					0x43
#define SYSEX_FORMAT_YAMAHA_32VOICES	0x09

#define PATCH_FILE_DIR					SOURCE_DIR "/res/patches"

const char *rom_names[PATCH_FILE_ROM_COUNT] = {
		"rom1a.syx",
};

static struct {
	bool is_loaded;
	patch_file_rom_t current_patch_file;
	uint8_t current_patch_number;
	voice_params_t current_voice_params[PATCH_FILE_NUM_VOICES];
} m_patch_file;

static enum {
	SYSEX_PARSE_STATE_STATUS_START,
	SYSEX_PARSE_STATE_ID,
	SYSEX_PARSE_STATE_SUB_STATUS_CHANNEL,
	SYSEX_PARSE_STATE_FORMAT,
	SYSEX_PARSE_STATE_BYTE_COUNT,
	SYSEX_PARSE_STATE_DATA,
	SYSEX_PARSE_STATE_CHECKSUM,
	SYSEX_PARSE_STATE_STATUS_END,
	SYSEX_PARSE_STATE_DONE,
} sysex_parse_state;

static struct {
	uint8_t id:7;
	uint8_t sub_status:3;
	uint8_t channel:4;
	uint8_t format:7;
	uint16_t byte_count:14;
	const uint8_t *p_data;
	uint8_t checksum:7;
} sysex_file;

static ret_code_t patch_file_decode(const uint8_t *p_data, uint32_t data_len, voice_params_t *p_voice_params);

ret_code_t patch_file_load_rom(patch_file_rom_t patch_file) {
	if (patch_file >= PATCH_FILE_ROM_COUNT) {
		log_error("Invalid patch file: %u", patch_file);
		return RET_CODE_ERROR;
	}

	if (m_patch_file.is_loaded && m_patch_file.current_patch_file == patch_file) {
		return RET_CODE_OK;
	}

	// Open file
	char file_path[256];
	sprintf(file_path, "%s/%s", PATCH_FILE_DIR, rom_names[patch_file]);
	FILE *file = fopen(file_path, "rb");
	if (!file) {
		log_error("Failed to open file");
		return RET_CODE_ERROR;
	}

	// Read file into buffer
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	uint8_t *buffer = malloc(file_size);
	if (!buffer) {
		log_error("Failed to allocate memory");
		return RET_CODE_ERROR;
	}
	fread(buffer, file_size, 1, file);
	fclose(file);

	// Parse buffer
	if (patch_file_decode(buffer, file_size, m_patch_file.current_voice_params) != RET_CODE_OK) {
		log_error("Failed to decode patch file");
		return RET_CODE_ERROR;
	}

	// Free buffer
	free(buffer);

	m_patch_file.current_patch_file = patch_file;
	m_patch_file.is_loaded = true;

	log_info("Loaded patch file: %s", rom_names[patch_file]);

	return RET_CODE_OK;
}

ret_code_t patch_file_load_rom_by_name(const char *rom_name) {
	for (uint32_t i = 0; i < PATCH_FILE_ROM_COUNT; i++) {
		if (strcmp(rom_name, rom_names[i]) == 0) {
			return patch_file_load_rom(i);
		}
	}

	log_error("Invalid patch file name: %s", rom_name);

	return RET_CODE_ERROR;
}

ret_code_t patch_file_load_patch(uint8_t patch_number, voice_params_t *voice_params) {
	if (!m_patch_file.is_loaded) {
		log_error("No patch file loaded");
		return RET_CODE_ERROR;
	}

	if (patch_number >= PATCH_FILE_NUM_VOICES) {
		log_error("Invalid voice number: %u", patch_number);
		return RET_CODE_ERROR;
	}

	memcpy(voice_params, &m_patch_file.current_voice_params[patch_number], sizeof(voice_params_t));

	m_patch_file.current_patch_number = patch_number;

	log_info("Loaded patch number: %u", patch_number);

	return RET_CODE_OK;
}

ret_code_t patch_file_load_patch_by_name(const char *patch_name, voice_params_t *voice_params) {
	if (!m_patch_file.is_loaded) {
		log_error("No patch file loaded");
		return RET_CODE_ERROR;
	}

	for (uint32_t i = 0; i < PATCH_FILE_NUM_VOICES; i++) {
		if (strncmp(patch_name, m_patch_file.current_voice_params[i].name, sizeof(m_patch_file.current_voice_params[0].name)) == 0) {
			return patch_file_load_patch(i, voice_params);
		}
	}

	log_error("Invalid patch name: %s", patch_name);

	return RET_CODE_ERROR;
}

bool patch_file_is_loaded(void) {
	return m_patch_file.is_loaded;
}

patch_file_rom_t patch_file_get_current_rom(void) {
	return m_patch_file.current_patch_file;
}

uint8_t patch_file_get_current_patch_number(void) {
	return m_patch_file.current_patch_number;
}

const char *patch_file_get_rom_names(void) {
	return (const char *) rom_names;
}

voice_params_t *patch_file_get_voice_params(void) {
	return m_patch_file.current_voice_params;
}

uint8_t twos_complement_checksum_7bit(const uint8_t *p_data, uint32_t data_len) {
	int sum = 0;

	for (uint32_t i = 0; i < data_len; i++) {
		sum -= p_data[i];
	}

	return sum & 0x7F;
}

ret_code_t yamaha_dx7_decode_voice_data(const uint8_t *p_data, uint32_t data_len, uint32_t *p_bytes_consumed, voice_params_t *p_voice_params) {
	if (data_len != 32 * 128) {
		log_error("Invalid data length: %u", data_len);
		return RET_CODE_ERROR;
	}

	uint32_t data_idx = 0;

	for (uint8_t voice_idx = 0; voice_idx < 32; voice_idx++) {
		voice_params_t *p_voice = &p_voice_params[voice_idx];

		for (uint8_t op_idx = 0; op_idx < 6; op_idx++) {
			operator_params_t *p_op = &p_voice->operators[6 - op_idx - 1];

			p_op->env.rate1 = p_data[data_idx++];
			p_op->env.rate2 = p_data[data_idx++];
			p_op->env.rate3 = p_data[data_idx++];
			p_op->env.rate4 = p_data[data_idx++];
			p_op->env.level1 = p_data[data_idx++];
			p_op->env.level2 = p_data[data_idx++];
			p_op->env.level3 = p_data[data_idx++];
			p_op->env.level4 = p_data[data_idx++];
			p_op->kls.break_point = p_data[data_idx++];
			p_op->kls.left_depth = p_data[data_idx++];
			p_op->kls.right_depth = p_data[data_idx++];
			p_op->kls.left_curve = p_data[data_idx] & 0x03;
			p_op->kls.right_curve = p_data[data_idx++] >> 2;
			p_op->keyboard_rate_scaling = p_data[data_idx] & 0x07;
			p_op->osc.detune = p_data[data_idx++] >> 3;
			p_op->amplitude_modulation_sensitivity = p_data[data_idx] & 0x03;
			p_op->key_velocity_sensitivity = p_data[data_idx++] >> 2;
			p_op->output_level = p_data[data_idx++];
			p_op->osc.mode = p_data[data_idx] & 0x01;
			p_op->osc.frequency_coarse = p_data[data_idx++] >> 1;
			p_op->osc.frequency_fine = p_data[data_idx++];
		}

		p_voice->pitch_eg.rate1 = p_data[data_idx++];
		p_voice->pitch_eg.rate2 = p_data[data_idx++];
		p_voice->pitch_eg.rate3 = p_data[data_idx++];
		p_voice->pitch_eg.rate4 = p_data[data_idx++];
		p_voice->pitch_eg.level1 = p_data[data_idx++];
		p_voice->pitch_eg.level2 = p_data[data_idx++];
		p_voice->pitch_eg.level3 = p_data[data_idx++];
		p_voice->pitch_eg.level4 = p_data[data_idx++];
		p_voice->algorithm = p_data[data_idx++];
		p_voice->feedback = p_data[data_idx] & 0x07;
		p_voice->oscillator_key_sync = p_data[data_idx++] >> 3;
		p_voice->lfo.speed = p_data[data_idx++];
		p_voice->lfo.delay = p_data[data_idx++];
		p_voice->lfo.pitch_modulation_depth = p_data[data_idx++];
		p_voice->lfo.amplitude_modulation_depth = p_data[data_idx++];
		p_voice->lfo.sync = p_data[data_idx] & 0x01;
		p_voice->lfo.wave = (p_data[data_idx] >> 1) & 0x0F;
		p_voice->lfo.pitch_modulation_sensitivity = p_data[data_idx++] >> 5;
		p_voice->transpose = p_data[data_idx++];

		for (uint8_t name_idx = 0; name_idx < 10; name_idx++) {
			p_voice->name[name_idx] = (char) p_data[data_idx++];
		}
	}

	*p_bytes_consumed += data_idx;

	return RET_CODE_OK;
}

ret_code_t patch_file_decode(const uint8_t *p_data, uint32_t data_len, voice_params_t *p_voice_params) {
	sysex_parse_state = SYSEX_PARSE_STATE_STATUS_START;

	uint32_t bytes_consumed = 0;

	while (bytes_consumed < data_len) {
		switch (sysex_parse_state) {
			case SYSEX_PARSE_STATE_STATUS_START:
				if (p_data[bytes_consumed] != SYSEX_STATUS_START) {
					log_error("Invalid sysex start status: %02X", p_data[bytes_consumed]);
					return RET_CODE_ERROR;
				}
				sysex_parse_state = SYSEX_PARSE_STATE_ID;
				bytes_consumed++;
				break;

			case SYSEX_PARSE_STATE_ID:
				sysex_file.id = p_data[bytes_consumed] & 0x7F;
				sysex_parse_state = SYSEX_PARSE_STATE_SUB_STATUS_CHANNEL;
				bytes_consumed++;
				if (sysex_file.id != SYSEX_ID_YAMAHA) {
					log_error("Sysex ID is not Yamaha: %02X", sysex_file.id);
					return RET_CODE_ERROR;
				}
				break;

			case SYSEX_PARSE_STATE_SUB_STATUS_CHANNEL:
				sysex_file.sub_status = (p_data[bytes_consumed] & 0x70) >> 4;
				sysex_file.channel = p_data[bytes_consumed] & 0x0F;
				sysex_parse_state = SYSEX_PARSE_STATE_FORMAT;
				bytes_consumed++;
				break;

			case SYSEX_PARSE_STATE_FORMAT:
				sysex_file.format = p_data[bytes_consumed] & 0x7F;
				sysex_parse_state = SYSEX_PARSE_STATE_BYTE_COUNT;
				bytes_consumed++;
				if (sysex_file.format != SYSEX_FORMAT_YAMAHA_32VOICES) {
					log_error("Sysex format is not Yamaha 32 voices: %02X", sysex_file.format);
					return RET_CODE_ERROR;
				}
				break;

			case SYSEX_PARSE_STATE_BYTE_COUNT:
				if (data_len - bytes_consumed < 2) {
					log_error("Not enough data for byte count");
					return RET_CODE_ERROR;
				}
				sysex_file.byte_count = (p_data[bytes_consumed] << 7) | p_data[bytes_consumed + 1];
				sysex_parse_state = SYSEX_PARSE_STATE_DATA;
				bytes_consumed += 2;
				break;

			case SYSEX_PARSE_STATE_DATA:
				sysex_file.p_data = &p_data[bytes_consumed];
				if (yamaha_dx7_decode_voice_data(p_data + bytes_consumed, data_len - bytes_consumed - 2, &bytes_consumed, p_voice_params) != RET_CODE_OK) {
					return RET_CODE_ERROR;
				}
				sysex_parse_state = SYSEX_PARSE_STATE_CHECKSUM;
				break;

			case SYSEX_PARSE_STATE_CHECKSUM:
				sysex_file.checksum = p_data[bytes_consumed] & 0x7F;
				sysex_parse_state = SYSEX_PARSE_STATE_STATUS_END;
				bytes_consumed++;
				if (twos_complement_checksum_7bit(sysex_file.p_data, sysex_file.byte_count) != sysex_file.checksum) {
					log_error("Checksum mismatch");
					return RET_CODE_ERROR;
				}
				break;

			case SYSEX_PARSE_STATE_STATUS_END:
				if (p_data[bytes_consumed] != SYSEX_STATUS_END) {
					log_error("Invalid sysex end status: %02X", p_data[bytes_consumed]);
					return RET_CODE_ERROR;
				}
				sysex_parse_state = SYSEX_PARSE_STATE_DONE;
				bytes_consumed++;
				break;

			case SYSEX_PARSE_STATE_DONE:
				log_error("Extra data after sysex end status");
				return RET_CODE_ERROR;

			default:
				log_error("Invalid sysex parse state: %u", sysex_parse_state);
				return RET_CODE_ERROR;
		}
	}

	if (sysex_parse_state != SYSEX_PARSE_STATE_DONE) {
		log_error("Incomplete sysex message");
		return RET_CODE_ERROR;
	}

	return RET_CODE_OK;
}
