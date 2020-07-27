/**
 * This file is part of UdpSender.
 *
 * UdpSender is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * UdpSender is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with UdpSender.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Pascal Bodin
 */

#include <stdbool.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "esp_log.h"

#include "messages.h"
#include "utilities.h"
#include "connect_wifi.h"

#define INPUT_QUEUE_LENGTH 3

#define SEND_PERIOD_MS 30000

static const char *TAG = "SD";

// Input queue.
QueueHandle_t sd_input_queue = NULL;

typedef enum {
	SD_WAIT_CONN_STATUS_ST,
	SD_WAIT_SEND_PERIOD_ST,
	SD_ERROR_ST,
} state_t;

static state_t current_state;

static void send_and_wait(uint8_t *payload_data, uint8_t payload_length,
		                  TimerHandle_t timer,
						  state_t *current_state) {

	message_t message_to_send;

	// Delay used for xTicksToWait when calling xTimerStart().
	const TickType_t delay_500ms = pdMS_TO_TICKS(500);

	// Send the send_datagram to connect_wifi task.
	message_to_send.message = CW_SEND_DATAGRAM;
	message_to_send.cw_send_datagram.payload = payload_data;
	message_to_send.cw_send_datagram.payload_length = payload_length;
	BaseType_t fr_rs = send_to_queue(cw_input_queue, &message_to_send, TAG);
	if (fr_rs != pdTRUE) {
		ESP_LOGE(TAG, "Error on sending message to connect_wifi - %d", fr_rs);
		send_error(SD_QUEUE_ERR, TAG);
		*current_state = SD_ERROR_ST;
		return;
	}
	// Then, wait for some time before sending another datagram. The event handler
	// called at timer timeout will send the CW_TIMEOUT message.
	fr_rs = xTimerStart(timer, delay_500ms);
	if (fr_rs != pdPASS) {
		ESP_LOGE(TAG, "Error from xTimerStart: %d", fr_rs);
		send_error(SD_TIMER_ERR, TAG);
		*current_state = SD_ERROR_ST;
		return;
	}

}

/**
 * Event handler for the timer.
 */
static void timer_handler(TimerHandle_t timer) {

	message_t message_to_send;
	message_to_send.message = SD_TIMEOUT;
	message_to_send.no_payload.nothing = 0;
    BaseType_t rs = send_to_queue(sd_input_queue, &message_to_send, TAG);
    if (rs != pdTRUE) {
    	ESP_LOGE(TAG, "timer_handler - error on sending message to myself - %d", rs);
    }

}

void send_datagram_task(void *pvParameters) {

	// Delay used for xTicksToWait when calling xQueueReceive().
	const TickType_t delay_60s = pdMS_TO_TICKS(60000);

	// The datagram payload is an ASCII message containing a counter.
	char payload_data[] = "This is message 000.";
	const uint8_t payload_counter_index = 16;
	const uint8_t payload_length = strlen(payload_data);
	uint8_t payload_counter;

	TimerHandle_t timer = NULL;

	BaseType_t fr_rs;  // Return status for FreeRTOS calls.

	message_t received_message;

	current_state = SD_WAIT_CONN_STATUS_ST;

	// Create our input queue, even if we are in error state.
	sd_input_queue = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(message_t));
	if (sd_input_queue == 0) {
		ESP_LOGE(TAG, "Error from xQueueCreate");
		send_error(SD_INIT_ERR, TAG);
		current_state = SD_ERROR_ST;
	}

	// Create the timer we'll use to send a datagram on a periodic basis.
	if (current_state != SD_ERROR_ST) {
		uint8_t timerID = 0;
		timer = xTimerCreate("SD_TIMER",
				pdMS_TO_TICKS(SEND_PERIOD_MS),
				pdFALSE,  // uxAutoReload.
				&timerID,
				timer_handler);
		if (timer == NULL) {
			ESP_LOGE(TAG, "Error from xTimerCreate");
			send_error(SD_INIT_ERR, TAG);
			current_state = SD_ERROR_ST;
		}
	}

	payload_counter = 0;

	while (true) {

		// Wait for an incoming message.
		fr_rs = xQueueReceive(sd_input_queue, &received_message, delay_60s);
		if (fr_rs != pdTRUE) {
			// Timeout. Go back to receive.
			ESP_LOGI(TAG, "Queue receive timeout");
			continue;
		}

		switch (current_state) {

		case SD_WAIT_CONN_STATUS_ST:
			if (received_message.message == SD_TIMEOUT) {
				// When the connection to the Internet is lost while we were already connected,
				// we go back to this state, and we can receive the timeout message related to
				// the send datagram period.
				ESP_LOGI(TAG, "SD_WAIT_CONN_STATUS_ST - timeout");
				break;
			}
			if (received_message.message == SD_CONNECTION_STATUS) {
				bool connected = received_message.sd_connection_status.connected;
				ESP_LOGI(TAG, "SD_WAIT_CONN_STATUS_ST - connection_status message received - %d",
						(uint8_t)connected);
				if (connected) {
					ESP_LOGI(TAG, "SD_WAIT_SEND_PERIOD_ST - sending a datagram - %03d", payload_counter);
					send_and_wait((uint8_t *)payload_data, payload_length,
							      timer, &current_state);
					if (current_state == SD_ERROR_ST) {
						break;
					}
					current_state = SD_WAIT_SEND_PERIOD_ST;
					break;
				}
				// At this stage, the message contains disconnected. Ignore it.
				ESP_LOGE(TAG, "SD_WAIT_CONN_STATUS_T - unexpected disconnected connection_status message");
				break;
			}
			ESP_LOGE(TAG, "SD_WAIT_CONN_STATUS_T - unexpected message received: %d",
					 received_message.message);
			break;

		case SD_WAIT_SEND_PERIOD_ST:
			if (received_message.message == SD_TIMEOUT) {
				// End of wait period, send a new datagram.
				if (payload_counter == 0xff) {
					payload_counter = 0;
				} else {
					payload_counter++;
				}
				ESP_LOGI(TAG, "SD_WAIT_SEND_PERIOD_ST - sending a datagram - %03d", payload_counter);
				payload_data[payload_counter_index + 2] = (uint8_t)'0' + payload_counter % 10;
				payload_data[payload_counter_index + 1] = (uint8_t)'0' + (payload_counter / 10) % 10;
				payload_data[payload_counter_index] = (uint8_t)'0' + (payload_counter / 100) % 10;
				send_and_wait((uint8_t *)payload_data, payload_length,
						      timer, &current_state);
				if (current_state == SD_ERROR_ST) {
					break;
				}
				break;
				// Stay in same state.
			}
			if (received_message.message == SD_CONNECTION_STATUS) {
				bool connected = received_message.sd_connection_status.connected;
				ESP_LOGI(TAG, "SD_WAIT_CONN_STATUS_ST - connection_status message received - %d",
									(uint8_t)connected);
				if (!connected) {
					// Connection to the Internet is no more available.
					current_state = SD_WAIT_CONN_STATUS_ST;
					break;
				}
				// At this stage, the message contains connected. Ignore it.
				ESP_LOGE(TAG, "SD_WAIT_SEND_PERIOD_ST - unexpected connected conntection_status message.");
				break;
			}
			// At this stage, unexpected message.
			ESP_LOGE(TAG, "SD_WAIT_SEND_PERIOD_ST - unexpected message received: %d",
					received_message.message);
			break;

		case SD_ERROR_ST:
			// Once we enter this state, we stay in it.
			ESP_LOGI(TAG, "SD_ERROR_ST");
			break;

		default:
			ESP_LOGE(TAG, "Unknown state: %d", current_state);
			send_error(SD_UKNOWN_STATE_ERR, TAG);
			current_state = SD_ERROR_ST;
		}

	}

}
