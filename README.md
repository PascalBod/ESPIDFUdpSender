# ESP-IDF UDP sender

## Overview

This application initiates a connection to a Wi-Fi access point. Once the connection 
is established, it sends a UDP datagram to a remote IP address and port.

When the connection is lost, the application tries to reconnect on a periodic basis.

There are two takeaways in this sample application:
* how to handle a connection to the Internet via a Wi-Fi access point, in a way that can easily be adapted to "rainy days", as Espressif says in [their documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html#event-handling)
* the use of a design pattern relying on non-blocking inter-task communication (see [this article](https://www.monblocnotes.org/node/1906) for a little bit more information)

## Target

The hardware target is an [ESP32-DevKitC](https://www.espressif.com/en/products/devkits/esp32-devkitc/overview) with an ESP32-WROVER-B module.

ESP-IDF release is `release/4.1`.

## Reference documents

* [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/release-v4.1/index.html)
* [Wi-Fi Driver](https://docs.espressif.com/projects/esp-idf/en/release-v4.1/api-guides/wifi.html)
* [General Notes About ESP-IDF Programming / Application startup](https://docs.espressif.com/projects/esp-idf/en/release-v4.1/api-guides/general-notes.html#application-startup)

## Configure

To configure the application, run `idf.py menuconfig`, and select **UdpSender Configuration**. Set following parameters:
* **WiFi SSID**: the SSID of the access point to be used
* **WiFi password**: associated password
* **Retry period, in ms**: period between two successive connection attempts, after the connection has been lost
* **IPV4 Address**: address of the host where to send datagrams
* **Port**: host port 

## Build and flash
 
To build, flash and monitor the output from the application, run:

```
idp.py -p <port> flash monitor
```

Replace `<port>` by the name of the serial-over-USB port.

To exit the serial monitor, type ``Ctrl-]``.

## End-to-end test

A simple configuration is to run an UDP server on a computer that is on the same LAN that the Wi-Fi access point.

Configure the application with the IP address of this computer. 

Then, if this computer runs Linux or macOS, enter the following command:

```
nc -u -lk 0.0.0.0 44444
```

Every received datagram will be displayed.

## Architecture

### Tasks

The application is built by composing *tasks*. A task is implemented as a FreeRTOS task, and has the following characteristics:
* the task is in one of a finite number of states at any given time
* the task can receive *messages*
* a message is sent to the task's *input queue*, a queue owned by the task
* the send operation is non blocking
* all received messages are processed by the task in order of reception (First In, First Out)
* the task changes from its current state to another one depending on each message
* after having processed a message, the task may generate one or more messages to some other task(s)
* when the task encounters an unrecoverable error, it sends a message to the supervisor task (see below) and enters an error state that it will never leave

Each task implements a finite state machine.

In order to be able to send a message to another task, a task must know the queue of the other task. In the current implementation, in order to keep it very simple, all queues are globally accessible.

A specific task, the *supervisor* task, is in charge of ensuring application consistency. It receives error messages from other tasks when these ones encounter unrecoverable errors, so that it can act on other tasks accordingly.

### Message protocol

It's up to every task to define its message protocol, namely the list of messages it accepts, and how it reacts to each of them. The possible reactions are defined as transitions from one state to another one, depending on the messages, and as actions that are performed when entering a new state.

### Application tasks

The application is made of three tasks:
* *connect_wifi*
* *send_datagram*
* *supervisor*

#### connect_wifi

The connect_wifi task is in charge of maintaining a connection to the Internet via a given Wi-Fi access point, and sending UDP datagrams to a remote host.

It accepts the following messages:
* *connect* - payload: the Wi-Fi access point to use
* *disconnect* - payload: none
* *send_datagram* - payload: a pointer to a byte array (datagram payload)

It generates the following messages:
* *connection_status* - payload: connection status - generated on a connection state change - sent to the send_datagram task
* *send_error* - payload: send error - generated on a send error - sent to the send_datagram task
* *internal_error* - payload: internal error - generated on an internal error - sent to the supervisor task

After having received the connect message, the task tries to connect to the designated Wi-Fi access point, and to get an IP address. Once this is done, it sends the connection_status message to the send_datagram task.

If the access to the Internet is lost, the task sends a connection_status message to the send_datagram task, tries to reconnect on a periodic basis and, if it succeeds, sends another connection_status message to the send_datagram task.

The connect_wifi task contains following states and transitions:
![](connect_wifi.svg)

Several transitions are not present in the diagram, in order to keep it simple:
* transitions leading to the error state
* transitions going from a task to itself, corresponding to ignored unexpected messages

The send_datagram message is accepted only when the task is in wait_msg_disconnect state.

#### send_datagram

The send_datagram task requests the transmission of a datagram to the remote host, on a periodic basis.

It accepts the following messages:
* *connection_status* - see connect_wifi task
* *send_error* - see connect_wifi task

It generates the following messages:
* *send_datagram* - see connect_wifi task
* *internal_error* - payload: internal error - generated on an internal error - sent to the supervisor task

After initialization, the task waits for a connection_status message informing it that access to the Internet is available. Upon reception of this message, it sends the send_datagram message to the connect_wifi task. Then, on a periodic basis, it sends another send_datagram message, until it receives a connection_status message saying that the access to the Internet is lost.

#### supervisor

The supervisor task starts the connect_wifi task and then sends the connect messages to it.

When it receives an internal_error message, it reacts depending on the origin of the error.

## License

UdpSender is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version. Check the `COPYING` file for 
more information.

