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

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "messages.h"
#include "supervisor.h"

BaseType_t send_to_queue(QueueHandle_t xQueue, const void * const pvItemToQueue,
		                 const char *TAG) {
	if (xQueue == NULL) {
		ESP_LOGW(TAG, "xQueue is NULL");
		return pdTRUE;
	}
	return xQueueSend(xQueue, pvItemToQueue, 0);
}

void send_error(sv_internal_error_type_t error, const char *TAG) {

	message_t message_to_send;
	// Send the error to the supervisor task.
	message_to_send.message = SV_INTERNAL_ERROR;
	message_to_send.sv_internal_error.error = error;
	BaseType_t rs = send_to_queue(sv_input_queue, &message_to_send, TAG);
	if (rs != pdTRUE) {
		ESP_LOGE(TAG, "Error on sending message to supervisor - %d", rs);
	}

}
