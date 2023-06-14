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

HTTP_ROUTE_METHOD(get_roms, HTTP_METHOD_GET, {
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
});

HTTP_ROUTE_METHOD(select_rom, HTTP_METHOD_POST, {
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
});

HTTP_ROUTE_METHOD(get_patches, HTTP_METHOD_GET, {
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
});

HTTP_ROUTE_METHOD(select_patch, HTTP_METHOD_POST, {
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
});

HTTP_ROUTE_METHOD(get_params, HTTP_METHOD_GET, {
	voice_params_t *p_params = &synth_data.voice_params;

	uint32_t string_length = 10*1024;
	char *json_string = malloc(string_length);
	uint32_t index = 0;
	index += snprintf(json_string + index, string_length - index, "{\"params\":{");
	index += snprintf(json_string + index, string_length - index, "\"operators\":[");
	for (uint8_t i = 0; i < NUM_OPERATORS_PER_VOICE; i++) {
		index += snprintf(json_string + index, string_length - index,
						  "{\"env\":{\"rate1\":%u,\"level1\":%u,\"rate2\":%u,\"level2\":%u,\"rate3\":%u,\"level3\":%u,\"rate4\":%u,\"level4\":%u},"
						  "\"kls\":{\"break_point\":%u,\"left_depth\":%u,\"right_depth\":%u,\"left_curve\":%u,\"right_curve\":%u},"
						  "\"osc\":{\"mode\":%u,\"frequency_coarse\":%u,\"frequency_fine\":%u,\"detune\":%u},"
						  "\"keyboard_rate_scaling\":%u,\"amplitude_modulation_sensitivity\":%u,\"key_velocity_sensitivity\":%u,\"output_level\":%u}",
		p_params->operators[i].env.rate1, p_params->operators[i].env.level1, p_params->operators[i].env.rate2, p_params->operators[i].env.level2,
		p_params->operators[i].env.rate3, p_params->operators[i].env.level3, p_params->operators[i].env.rate4, p_params->operators[i].env.level4,
		p_params->operators[i].kls.break_point, p_params->operators[i].kls.left_depth, p_params->operators[i].kls.right_depth,
		p_params->operators[i].kls.left_curve, p_params->operators[i].kls.right_curve, p_params->operators[i].osc.mode,
		p_params->operators[i].osc.frequency_coarse, p_params->operators[i].osc.frequency_fine, p_params->operators[i].osc.detune,
		p_params->operators[i].keyboard_rate_scaling, p_params->operators[i].amplitude_modulation_sensitivity,
		p_params->operators[i].key_velocity_sensitivity, p_params->operators[i].output_level);
		if (i < NUM_OPERATORS_PER_VOICE - 1) {
			index += snprintf(json_string + index, string_length - index, ",");
		}
	}
	index += snprintf(json_string + index, string_length - index, "],");
	index += snprintf(json_string + index, string_length - index,
					  "\"pitch_eg\":{\"rate1\":%u,\"level1\":%u,\"rate2\":%u,\"level2\":%u,\"rate3\":%u,\"level3\":%u,\"rate4\":%u,\"level4\":%u},"
					  "\"lfo\":{\"speed\":%u,\"delay\":%u,\"pitch_modulation_depth\":%u,\"amplitude_modulation_depth\":%u,\"sync\":%u,\"wave\":%u,\"pitch_modulation_sensitivity\":%u},"
					  "\"algorithm\":%u,\"feedback\":%u,\"oscillator_key_sync\":%u,\"transpose\":%u,\"name\":\"%s\"}",
	p_params->pitch_eg.rate1, p_params->pitch_eg.level1, p_params->pitch_eg.rate2, p_params->pitch_eg.level2,
	p_params->pitch_eg.rate3, p_params->pitch_eg.level3, p_params->pitch_eg.rate4, p_params->pitch_eg.level4,
	p_params->lfo.speed, p_params->lfo.delay, p_params->lfo.pitch_modulation_depth, p_params->lfo.amplitude_modulation_depth,
	p_params->lfo.sync, p_params->lfo.wave, p_params->lfo.pitch_modulation_sensitivity,
	p_params->algorithm, p_params->feedback, p_params->oscillator_key_sync, p_params->transpose, p_params->name);
	index += snprintf(json_string + index, string_length - index, "}");

	response.json(json_string);
	
	free(json_string);
});

HTTP_ROUTE_METHOD(midi_message, HTTP_METHOD_POST, {
	const char *body = request.body();

	json_object_t json_object;
	if (json_parse(body, strlen(body), &json_object) != RET_CODE_OK) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		return;
	}

	json_object_member_t *p_type = json_object_get_member(&json_object, "type");
	if (p_type == NULL || p_type->type != JSON_VALUE_TYPE_STRING) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	json_object_member_t *p_event = json_object_get_member(&json_object, "event");
	if (p_event == NULL || p_event->type != JSON_VALUE_TYPE_STRING) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	json_object_member_t *p_channel = json_object_get_member(&json_object, "channel");
	if (p_channel == NULL || p_channel->type != JSON_VALUE_TYPE_NUMBER) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	json_object_member_t *p_key = json_object_get_member(&json_object, "key");
	if (p_key == NULL || p_key->type != JSON_VALUE_TYPE_NUMBER) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	json_object_member_t *p_velocity = json_object_get_member(&json_object, "velocity");
	if (p_velocity == NULL || p_velocity->type != JSON_VALUE_TYPE_NUMBER) {
		response.text("Invalid JSON");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	uint8_t channel = p_channel->value.number;
	uint8_t key = p_key->value.number;
	uint8_t velocity = p_velocity->value.number;

	if (strcmp(p_type->value.string, "voice_message") == 0) {
		if (strcmp(p_event->value.string, "note_on") == 0) {
			voice_assign_key(key, velocity);
		} else if (strcmp(p_event->value.string, "note_off") == 0) {
			voice_release_key(key, velocity);
		} else {
			response.text("Unhandled event");
			response.status(HTTP_STATUS_CODE_BAD_REQUEST);
			json_object_free(&json_object);
			return;
		}
	} else {
		response.text("Unhandled type");
		response.status(HTTP_STATUS_CODE_BAD_REQUEST);
		json_object_free(&json_object);
		return;
	}

	json_object_free(&json_object);
});

ret_code_t web_server_start(void) {
	// Route
	server.websocket_streaming("api/viz_stream", visualization_route_websocket_stream);
	server.route("api/get_roms", get_roms);
	server.route("api/select_rom", select_rom);
	server.route("api/get_patches", get_patches);
	server.route("api/select_patch", select_patch);
	server.route("api/get_params", get_params);
	server.route("api/midi_message", midi_message);
	server.serve_static("static");

	// Start
	server.run();

	return RET_CODE_OK;
}
