//
// Created by Tim Holzhey on 14.06.23
//

#include <stdio.h>
#include <string.h>
#include "common.h"
#include "synthesizer.h"
#include "audio_driver.h"
#include "web_server.h"

int main(void) {
	if (synthesizer_init() != RET_CODE_OK) {
		log_error("Failed to initialize synthesizer.")
		return 1;
	}

	if (audio_driver_start() != RET_CODE_OK) {
		log_error("Failed to initialize audio driver.")
		return 1;
	}

	if (web_server_start() != RET_CODE_OK) {
		log_error("Failed to start web server.")
		return 1;
	}

	if (audio_driver_stop() != RET_CODE_OK) {
		log_error("Failed to stop audio driver.")
		return 1;
	}

	return 0;
}
