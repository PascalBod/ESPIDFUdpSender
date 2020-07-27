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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "esp_log.h"

#include "messages.h"
#include "connect_wifi.h"
#include "utilities.h"

#define INPUT_QUEUE_LENGTH 3

#define WAIT_TASKS_DELAY_MS 1000

static const char *TAG = "SV";

// SSID and password of the access point to be used must
// be configured by the configuration utility before building
// the application.
static char *SSID = CONFIG_UDPSENDER_WIFI_SSID;
static char *PASSWORD = CONFIG_UDPSENDER_WIFI_PASSWORD;

// Input queue.
QueueHandle_t sv_input_queue;

typedef enum {
	SV_WAIT_DELAY_ST,
	SV_WAIT_MSG_ST,
	SV_ERROR_ST,
} state_t;

static state_t current_state;

/**
 * Event handler for the timer.
 */
static void timer_handler(TimerHandle_t timer) {

	message_t message_to_send;
	message_to_send.message = SV_TIMEOUT;
	message_to_send.no_payload.nothing = 0;
    BaseType_t rs = send_to_queue(sv_input_queue, &message_to_send, TAG);
    if (rs != pdTRUE) {
    	ESP_LOGE(TAG, "timer_handler - error on sending message to myself - %d", rs);
    }

}

void supervisor_task(void *pvParameters) {

	const TickType_t ready_delay = pdMS_TO_TICKS(WAIT_TASKS_DELAY_MS);

	const TickType_t delay_60s = pdMS_TO_TICKS(60000);

	const TickType_t block_delay = pdMS_TO_TICKS(500);

	TimerHandle_t timer = NULL;

	message_t received_message;
	message_t message_to_send;

	BaseType_t fr_rs;   // Return status for FreeRTOS calls.
	BaseType_t esp_rs;  // Return status for ESP-IDF calls.

	current_state = SV_WAIT_DELAY_ST;

	sv_input_queue = xQueueCreate(INPUT_QUEUE_LENGTH, sizeof(message_t));
	if (sv_input_queue == 0) {
		ESP_LOGE(TAG, "Error from xQueueCreate");
		current_state = SV_ERROR_ST;
	}

	if (current_state != SV_ERROR_ST) {
		// Prepare to wait for some time, for other tasks to be created by FreeRTOS.
		uint8_t timerID = 0;
		timer = xTimerCreate("SV_TIMER",
				ready_delay,
				pdFALSE,  // uxAutoReload.
				&timerID,
				timer_handler);
		if (timer == NULL) {
			ESP_LOGE(TAG, "Error from xTimerCreate");
			current_state = SV_ERROR_ST;
		}
	}
	if (current_state != SV_ERROR_ST) {
		fr_rs = xTimerStart(timer, block_delay);
		if (fr_rs != pdPASS) {
			ESP_LOGE(TAG, "Error from xTimerStart: %d", fr_rs);
			current_state = SV_ERROR_ST;
		}
	}

	while (true) {

		// Wait for an incoming message.
		fr_rs = xQueueReceive(sv_input_queue, &received_message, delay_60s);
		if (fr_rs != pdTRUE) {
			// Timeout. Go back to receive.
			ESP_LOGI(TAG, "Queue receive timeout");
			continue;
		}

		switch (current_state) {

		case SV_WAIT_DELAY_ST:
			if (received_message.message != SV_TIMEOUT) {
				// Unexpected message, ignore it, stay in this state.
				ESP_LOGE(TAG, "SV_WAIT_DELAY_ST - unexpected message received: %d", received_message.message);
				break;
			}
			// Tell connect_wifi task to connect to the AP.
			message_to_send.message = CW_CONNECT;
			message_to_send.cw_connect.ssid = SSID;
			message_to_send.cw_connect.password = PASSWORD;
			// Send message to connect_wifi task. Send operation
			// performs a copy. We do not wait (xTicksToWait = 0).
			esp_rs = send_to_queue(cw_input_queue, &message_to_send, TAG);
			if (esp_rs != pdTRUE) {
				ESP_LOGE(TAG, "Error on sending message to connect_wifi - %d", esp_rs);
				current_state = SV_ERROR_ST;
				break;
			}
			// At this stage, message sent to connect_wifi task.
			ESP_LOGI(TAG, "Message sent");
			current_state = SV_WAIT_MSG_ST;
			break;

		case SV_WAIT_MSG_ST:
			if (received_message.message == SV_INTERNAL_ERROR) {
				ESP_LOGI(TAG, "SV_INTERNAL_ERROR message: %d", received_message.sv_internal_error.error);
				break;
			}
			ESP_LOGE(TAG, "SV_WAIT_MSG_ST - unexpected message received: %d", received_message.message);
			break;

		case SV_ERROR_ST:
			// This state is entered after the occurrence of an internal error,
			// not of an error from another task.
			break;

		default:
			ESP_LOGE(TAG, "Unknown state: %d", current_state);
			current_state = SV_ERROR_ST;
		}

	}

}
