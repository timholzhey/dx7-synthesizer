//
// Created by Tim Holzhey on 14.06.23
//

#include "audio_driver.h"
#include "portaudio.h"
#include "config.h"
#include "synthesizer.h"

static PaStream *stream;

ret_code_t audio_driver_start(void) {
	PaError err;

	err = Pa_Initialize();
	if (err != paNoError) return RET_CODE_ERROR;

	PaStreamParameters outputParameters;
	outputParameters.device = Pa_GetDefaultOutputDevice();
	if (outputParameters.device == paNoDevice) {
		log_error("Error: No default output device.");
		return RET_CODE_ERROR;
	}
	outputParameters.channelCount = 2;
	outputParameters.sampleFormat = paInt32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	err = Pa_OpenStream(
			&stream,
			NULL,
			&outputParameters,
			AUDIO_SAMPLE_RATE,
			AUDIO_FRAMES_PER_BUFFER,
			paClipOff,
			synthesizer_render,
			&synth_data);
	if (err != paNoError) return RET_CODE_ERROR;

	err = Pa_StartStream(stream);
	if (err != paNoError) return RET_CODE_ERROR;

	return RET_CODE_OK;
}

ret_code_t audio_driver_stop(void) {
	PaError err;

	err = Pa_StopStream(stream);
	if (err != paNoError) return RET_CODE_ERROR;

	err = Pa_CloseStream(stream);
	if (err != paNoError) return RET_CODE_ERROR;

	Pa_Terminate();

	return RET_CODE_OK;
}
