//
// Created by Tim Holzhey on 14.06.23
//

#include "voice.h"
#include <stdbool.h>
#include "synthesizer.h"

bool voice_active[NUM_VOICES];

void voice_init(void) {
	for (uint8_t i = 0; i < NUM_VOICES; i++) {
		voice_active[i] = false;
		synth_data.voice_data[i].gate = 0;
		synth_data.voice_data[i].note = 0;
	}
}

ret_code_t voice_assign_key(uint8_t midi_key, uint8_t velocity) {
	for (uint8_t i = 0; i < NUM_VOICES; i++) {
		if (voice_active[i] && synth_data.voice_data[i].note == midi_key) {
			return RET_CODE_OK;
		}
	}

	bool found = false;
	for (uint8_t i = 0; i < NUM_VOICES; i++) {
		if (!voice_active[i]) {
			voice_active[i] = true;
			synth_data.voice_data[i].enable = 1;
			synth_data.voice_data[i].gate = 1;
			synth_data.voice_data[i].note = midi_key;
			for (uint32_t j = 0; j < NUM_OPERATORS; j++) {
				synth_data.voice_data[i].operator_data[j].envelope_data.state = ENVELOPE_STATE_ATTACK;
				synth_data.voice_data[i].operator_data[j].envelope_data.level = 0;
				synth_data.voice_data[i].operator_data[j].phase = 0;
				synth_data.voice_data[i].operator_data[j].input_mod_buffer = 0;
			}
			found = true;
			break;
		}
	}

	if (!found) {
		log_error("No free voice found.")
		return RET_CODE_ERROR;
	}

	return RET_CODE_OK;
}

void voice_release_key(uint8_t midi_key, uint8_t velocity) {
	for (uint8_t i = 0; i < NUM_VOICES; i++) {
		if (voice_active[i] && synth_data.voice_data[i].note == midi_key) {
			synth_data.voice_data[i].gate = 0;
			voice_active[i] = false;
			break;
		}
	}
}
