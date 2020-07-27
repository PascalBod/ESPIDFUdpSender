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
#ifndef MAIN_SEND_DATAGRAM_H_
#define MAIN_SEND_DATAGRAM_H_

#include "freertos/queue.h"

extern QueueHandle_t sd_input_queue;

void send_datagram_task(void *pvParameters);

#endif /* MAIN_SEND_DATAGRAM_H_ */
