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

#ifndef MAIN_UTILITIES_H_
#define MAIN_UTILITIES_H_

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum {
	QUEUE_OK,
	QUEUE_FULL,
	QUEUE_NULL,
	QUEUE_ERR
} queue_rs_t;

/**
 * Wrapper for xQueueSend(), which tests xQueue. If xQueue is NULL, a warning
 * message is printed.
 */
queue_rs_t send_to_queue(QueueHandle_t xQueue, const void * const pvItemToQueue,
		                 const char *TAG);

/**
 * Sends the error to the supervisor task.
 */
void send_error(sv_internal_error_type_t error, const char *TAG);

#endif /* MAIN_UTILITIES_H_ */
