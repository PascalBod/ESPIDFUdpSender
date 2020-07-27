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

#ifndef MAIN_MESSAGES_H_
#define MAIN_MESSAGES_H_

#include <stdbool.h>
#include <stdint.h>

// List of message types.
typedef enum {
	CW_CONNECT,
	CW_DISCONNECT,
	CW_SEND_DATAGRAM,
	CW_STA_OK,  // For internal use.
	CW_IP_OK,  // For internal use.
	CW_AP_NOK, // For internal use.
	CW_TIMEOUT, // For internal use.
	SD_CONNECTION_STATUS,
	SD__SEND_ERROR,
	SD_TIMEOUT,  // For internal used.
	SV_TIMEOUT,  // For internal use.
	SV_INTERNAL_ERROR,
} message_type_t;

//========================================
// For CW_CONNECT message.
typedef struct {
	char *ssid;
	char *password;
} cw_connect_t;

//========================================
// For CW_SEND_DATAGRAM message.
typedef struct {
	uint8_t *payload;
	uint16_t payload_length;
} cw_send_datagram_t;

//========================================
// For SD_CONNECTION_STATUS message.
typedef struct {
	bool connected;
} sd_connection_status_t;

//========================================
// For SD_SEND_ERROR message.
typedef enum {
	CW_ERR_1,
} sd_send_error_type_t;

typedef struct {
	sd_send_error_type_t error;
} sd_send_error_t;

//========================================
// For SV_INTERNAL_ERROR message.
typedef enum {
	CW_INIT_ERR,
	CW_START_ERR,
	CW_CONNECT_ERR,
	CW_IP_ERR,
	CW_DISCONNECT_ERR,
	CW_TIMER_ERR,
	CW_QUEUE_ERR,
	CW_UKNOWN_STATE_ERR,
	SD_INIT_ERR,
	SD_QUEUE_ERR,
	SD_TIMER_ERR,
	SD_UKNOWN_STATE_ERR,
} sv_internal_error_type_t;

typedef struct {
	sv_internal_error_type_t error;
} sv_internal_error_t;

//========================================
// For messages with no payload.
typedef struct {
	uint8_t nothing;
} no_payload_t;

//========================================
// Message.
typedef struct {
	message_type_t message;
	union {
		cw_connect_t cw_connect;
		cw_send_datagram_t cw_send_datagram;
		sd_connection_status_t sd_connection_status;
		sd_send_error_t sd_send_error;
		sv_internal_error_t sv_internal_error;
		no_payload_t no_payload;
	};
} message_t;

#endif /* MAIN_MESSAGES_H_ */
