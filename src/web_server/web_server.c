//
// Created by Tim Holzhey on 14.06.23
//

#include <stdlib.h>
#include "web_server.h"
#include "http_server.h"
#include "visualization.h"
#include "json.h"
#include "http_status.h"
#include "patch_file.h"
#include "synthesizer.h"
#include "voice.h"

HTTP_SERVER(server);

HTTP_ROUTE_METHOD("/api/get_roms", get_roms, HTTP_METHOD_GET) {
	json_object_t json_object = {0};
	const char **rom_names = (const char **) patch_file_get_rom_names();

	json_value_t roms_value = {0};
	if (json_value_from_string_array(&roms_value, *rom_names, PATCH_FILE_ROM_COUNT) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	if (json_object_add_value(&json_object, "roms", roms_value, JSON_VALUE_TYPE_ARRAY) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	json_value_t is_loaded_value = {0};
	bool is_loaded = patch_file_is_loaded();
	is_loaded_value.boolean = false;
	if (json_object_add_value(&json_object, "is_loaded", is_loaded_value, JSON_VALUE_TYPE_BOOLEAN) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	json_value_t current_rom_value = {0};
	const char *current_rom = is_loaded ? rom_names[patch_file_get_current_rom()] : "";
	current_rom_value.string = malloc(strlen(current_rom) + 1);
	strcpy(current_rom_value.string, current_rom);

	if (json_object_add_value(&json_object, "current_rom", current_rom_value, JSON_VALUE_TYPE_STRING) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	char *json_string = json_stringify(&json_object);
	response.json(json_string);

	free(json_string);
	json_object_free(&json_object);
}

HTTP_ROUTE_METHOD("api/select_roms", select_rom, HTTP_METHOD_POST) {
	const char *body = request.body();

	json_object_t json_object;
	if (json_parse(body, strlen(body), &json_object) != RET_CODE_OK) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		return;
	}

	json_object_member_t *p_patch_file = json_object_get_member(&json_object, "rom");
	if (p_patch_file == NULL || p_patch_file->type != JSON_VALUE_TYPE_STRING) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	if (patch_file_load_rom_by_name(p_patch_file->value.string) != RET_CODE_OK) {
		response.text("Invalid ROM");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	json_object_free(&json_object);
}

HTTP_ROUTE_METHOD("api/get_patches", get_patches, HTTP_METHOD_GET) {
	if (!patch_file_is_loaded()) {
		response.text("No ROM loaded");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		return;
	}

	json_object_t json_object = {0};
	voice_params_t *p_voice_params = (voice_params_t *) patch_file_get_voice_params();

	char patch_names[PATCH_FILE_NUM_VOICES][sizeof(p_voice_params[0].name) + 1];
	for (uint8_t i = 0; i < PATCH_FILE_NUM_VOICES; i++) {
		strncpy(patch_names[i], p_voice_params[i].name, sizeof(p_voice_params[i].name));
		patch_names[i][sizeof(p_voice_params[i].name)] = '\0';
	}

	json_value_t patches_value = {0};
	if (json_value_from_string_array(&patches_value, (const char *) patch_names, PATCH_FILE_NUM_VOICES) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	if (json_object_add_value(&json_object, "patches", patches_value, JSON_VALUE_TYPE_ARRAY) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	json_value_t is_loaded_value = {0};
	is_loaded_value.boolean = true;
	if (json_object_add_value(&json_object, "is_loaded", is_loaded_value, JSON_VALUE_TYPE_BOOLEAN) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	json_value_t current_patch_value = {0};
	char *current_patch = patch_names[patch_file_get_current_patch_number()];
	current_patch_value.string = malloc(strlen(current_patch) + 1);
	strcpy(current_patch_value.string, current_patch);

	if (json_object_add_value(&json_object, "current_patch", current_patch_value, JSON_VALUE_TYPE_STRING) != RET_CODE_OK) {
		response.status(HTTP_STATUS_CODE_INTERNAL_SERVER_ERROR);
		return;
	}

	char *json_string = json_stringify(&json_object);
	response.json(json_string);

	free(json_string);
	json_object_free(&json_object);
}

HTTP_ROUTE_METHOD("api/select_patch", select_patch, HTTP_METHOD_POST) {
	const char *body = request.body();

	json_object_t json_object;
	if (json_parse(body, strlen(body), &json_object) != RET_CODE_OK) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		return;
	}

	json_object_member_t *p_patch = json_object_get_member(&json_object, "patch");
	if (p_patch == NULL || p_patch->type != JSON_VALUE_TYPE_STRING) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	if (patch_file_load_patch_by_name(p_patch->value.string, &synth_data.voice_params) != RET_CODE_OK) {
		response.text("Invalid patch");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	json_object_free(&json_object);
}

HTTP_ROUTE_METHOD("api/get_params", get_params, HTTP_METHOD_GET) {
	voice_params_t *p_params = &synth_data.voice_params;

	uint32_t string_length = 10*1024;
	char *json_string = malloc(string_length);
	uint32_t index = 0;
	index += snprintf(json_string + index, string_length - index, "{\"params\":{");
	index += snprintf(json_string + index, string_length - index, "\"operators\":[");
	for (uint8_t i = 0; i < NUM_OPERATORS; i++) {
		index += snprintf(json_string + index, string_length - index,
						  "{\"env\":{\"rates[0]\":%u,\"levels[0]\":%u,\"rates[1]\":%u,\"levels[1]\":%u,\"rates[2]\":%u,\"levels[2]\":%u,\"rates[3]\":%u,\"levels[3]\":%u},"
						  "\"kls\":{\"break_point\":%u,\"left_depth\":%u,\"right_depth\":%u,\"left_curve\":%u,\"right_curve\":%u},"
						  "\"osc\":{\"mode\":%u,\"frequency_coarse\":%u,\"frequency_fine\":%u,\"detune\":%u},"
						  "\"keyboard_rate_scaling\":%u,\"amplitude_modulation_sensitivity\":%u,\"key_velocity_sensitivity\":%u,\"output_level\":%u}",
		p_params->operators[i].env.rates[0], p_params->operators[i].env.levels[0], p_params->operators[i].env.rates[1], p_params->operators[i].env.levels[1],
		p_params->operators[i].env.rates[2], p_params->operators[i].env.levels[2], p_params->operators[i].env.rates[3], p_params->operators[i].env.levels[3],
		p_params->operators[i].kls.break_point, p_params->operators[i].kls.left_depth, p_params->operators[i].kls.right_depth,
		p_params->operators[i].kls.left_curve, p_params->operators[i].kls.right_curve, p_params->operators[i].osc.mode,
		p_params->operators[i].osc.frequency_coarse, p_params->operators[i].osc.frequency_fine, p_params->operators[i].osc.detune,
		p_params->operators[i].keyboard_rate_scaling, p_params->operators[i].amplitude_modulation_sensitivity,
		p_params->operators[i].key_velocity_sensitivity, p_params->operators[i].output_level);
		if (i < NUM_OPERATORS - 1) {
			index += snprintf(json_string + index, string_length - index, ",");
		}
	}
	index += snprintf(json_string + index, string_length - index, "],");
	index += snprintf(json_string + index, string_length - index,
					  "\"pitch_eg\":{\"rates[1]\":%u,\"levels[1]\":%u,\"rates[2]\":%u,\"levels[2]\":%u,\"rates[3]\":%u,\"levels[3]\":%u,\"rates[4]\":%u,\"levels[4]\":%u},"
					  "\"lfo\":{\"speed\":%u,\"delay\":%u,\"pitch_modulation_depth\":%u,\"amplitude_modulation_depth\":%u,\"sync\":%u,\"wave\":%u,\"pitch_modulation_sensitivity\":%u},"
					  "\"algorithm\":%u,\"feedback\":%u,\"oscillator_key_sync\":%u,\"transpose\":%u,\"name\":\"%s\"}",
	p_params->pitch_eg.rates[0], p_params->pitch_eg.levels[0], p_params->pitch_eg.rates[1], p_params->pitch_eg.levels[1],
	p_params->pitch_eg.rates[2], p_params->pitch_eg.levels[2], p_params->pitch_eg.rates[3], p_params->pitch_eg.levels[3],
	p_params->lfo.speed, p_params->lfo.delay, p_params->lfo.pitch_modulation_depth, p_params->lfo.amplitude_modulation_depth,
	p_params->lfo.sync, p_params->lfo.wave, p_params->lfo.pitch_modulation_sensitivity,
	p_params->algorithm, p_params->feedback, p_params->oscillator_key_sync, p_params->transpose, p_params->name);
	index += snprintf(json_string + index, string_length - index, "}");

	response.json(json_string);
	
	free(json_string);
}

WEBSOCKET_ROUTE("api/midi", midi) {
	switch (websocket.event) {
		case WEBSOCKET_EVENT_DATA:
			printf("MIDI message: %02X %02X %02X\n", websocket.data[0], websocket.data[1], websocket.data[2]);
			if (websocket.data_length != 3) {
				log_error("Invalid MIDI message length: %u", websocket.data_length);
				return;
			}
			uint8_t status = websocket.data[0] & 0xF0;
			uint8_t channel = websocket.data[0] & 0x0F;
			uint8_t data1 = websocket.data[1];
			uint8_t data2 = websocket.data[2];

			// Reject not note on/off messages
			if (status != 0x80 && status != 0x90) {
				log_error("Unhandled MIDI message: %02X", status);
				return;
			}

			if (status == 0x80) {
				// Note off
				voice_release_key(data1, data2);
			} else {
				// Note on
				voice_assign_key(data1, data2);
			}
			break;
		default:
			break;
	}
}

HTTP_ROUTE_METHOD("/api/viz_stream", visualization_stream, HTTP_METHOD_GET) {
	uint8_t *p_transfer_buffer = NULL;
	uint32_t transfer_buffer_size = 0;
	if (visualization_consume_transfer(&p_transfer_buffer, &transfer_buffer_size) == RET_CODE_OK) {
		char chunk_size[16];
		sprintf(chunk_size, "%x\r\n", transfer_buffer_size);

		response.append((uint8_t *) chunk_size, strlen(chunk_size));
		response.append((uint8_t *) p_transfer_buffer, transfer_buffer_size);
		response.append((uint8_t *) "\r\n", 2);

		http_headers_set_value_string(response.p_headers, response.p_num_headers, "Transfer-Encoding", "chunked");
		http_headers_set_value_string(response.p_headers, response.p_num_headers, "Content-Type", "application/octet-stream");
		http_headers_unset(response.p_headers, response.p_num_headers, "Content-Length");
		response.send();
	}
}

HTTP_ROUTE_METHOD("/api/init", init, HTTP_METHOD_POST) {
	voice_init();
}

ret_code_t web_server_start(void) {
	visualization_stream.streaming = true;
	http_route_t routes[] = {
			get_roms,
			select_rom,
			get_patches,
			select_patch,
			get_params,
			midi,
			visualization_stream,
			init
	};
	server.routes(routes, sizeof(routes) / sizeof(http_route_t));
	server.serve_static("static");
	server.hook(voice_update);

	// Blocking call
	server.run();

	return RET_CODE_OK;
}
