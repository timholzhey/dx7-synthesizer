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
	bool samples_streaming;
} m_viz;

void visualization_route_websocket_stream(void) {
	websocket_interface_t websocket;
	websocket_interface_init(&websocket);

	switch (websocket.event) {
		case WEBSOCKET_EVENT_DATA:
			if (strlen((char *) websocket.data) == 5 && strncmp((char *) websocket.data, "start", 5) == 0) {
				m_viz.samples_streaming = true;
				break;
			}
			if (strlen((char *) websocket.data) == 4 && strncmp((char *) websocket.data, "stop", 4) == 0) {
				m_viz.samples_streaming = false;
				break;
			}
			log_info("Unknown websocket data: %s", websocket.data);
			break;
		case WEBSOCKET_EVENT_DISCONNECTED:
			m_viz.samples_streaming = false;
			break;
		case WEBSOCKET_EVENT_NONE:
			if (m_viz.samples_streaming && m_viz.transfer_pending) {
				websocket.send((uint8_t *) m_viz.samples, m_viz.transfer_buffer_size * sizeof(int32_t));
				m_viz.transfer_pending = false;
			}
			return;
		default:
			break;
	}
}

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
