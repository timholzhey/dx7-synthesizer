//
// Created by Tim Holzhey on 14.06.23
//

#include "visualization.h"
#include "config.h"
#include "stdbool.h"
#include "http_server.h"

#define VIZ_SAMPLES_PER_SECOND             60
#define VIZ_SAMPLE_COUNT                   (AUDIO_SAMPLE_RATE / VIZ_SAMPLES_PER_SECOND)

static struct {
	int32_t samples[VIZ_SAMPLE_COUNT];
	uint32_t sample_index;

	int32_t transfer_buffer[VIZ_SAMPLE_COUNT];
	uint32_t transfer_buffer_size;
	bool transfer_pending;
} m_viz;

void visualization_add_sample(int32_t sample, uint32_t align_freq) {
	m_viz.samples[m_viz.sample_index++] = sample;

	if (m_viz.sample_index >= VIZ_SAMPLE_COUNT) {
		if (!m_viz.transfer_pending) {
			m_viz.transfer_pending = true;
			m_viz.transfer_buffer_size = m_viz.sample_index;
			memcpy(m_viz.transfer_buffer, m_viz.samples, m_viz.transfer_buffer_size * sizeof(float));
		}
		m_viz.sample_index = 0;
	}
}

ret_code_t visualization_consume_transfer(uint8_t **op_buffer, uint32_t *p_buffer_size) {
	if (!m_viz.transfer_pending) {
		return RET_CODE_ERROR;
	}

	*p_buffer_size = m_viz.transfer_buffer_size * sizeof(float);
	*op_buffer = (uint8_t *)m_viz.transfer_buffer;
	m_viz.transfer_pending = false;

	return RET_CODE_OK;
}
