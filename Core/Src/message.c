#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <slips.h>
#include <pb_decode.h>
#include "stm32f1xx_hal.h"
#include "message.h"
#include "status.pb.h"
#include "main.h"
#include "cmsis_os.h"

#define MAX_BUFFER_SIZE 1024

extern osMessageQueueId_t messageQueueHandle;

static bool slip_packet_continuation;
static uint8_t *slip_packet_buffer;
static uint32_t slip_packet_buffer_length;
static uint32_t slip_packet_buffer_index;
static unsigned int recv_char_call_count;
static char payload_buffer[MAX_BUFFER_SIZE];
static size_t payload_buffer_index;

static bool message_isr_recv_char(char *c, void *user_data) {
    recv_char_call_count++;

    if (slip_packet_buffer_index >= slip_packet_buffer_length)
        return false;

    *c = (char)(slip_packet_buffer[slip_packet_buffer_index++]);
    return true;
}

static bool message_isr_write_char(char c, void *user_data) {
    if (payload_buffer_index >= MAX_BUFFER_SIZE)
        return false;
    
    payload_buffer[payload_buffer_index++] = c;
    return true;
}

static slips_recv_context_t slips_context = {
    .decode_recv_char_fn = message_isr_recv_char,
    .decode_write_char_fn = message_isr_write_char,
    .check_start = true
};

static void message_isr_handle_protobuf_message(char *buffer, uint16_t length) {
    Status message = Status_init_default;
	pb_istream_t stream = pb_istream_from_buffer((const uint8_t *)buffer, length);

	if (pb_decode(&stream, &Status_msg, &message)) {
		switch (osMessageQueuePut(messageQueueHandle, &message, 0, 0)) {
		case osOK:
			printf("Info: Got message OK\n");
			break;
		case osErrorResource:
			printf("Error: Message queue is full\n");
			break;
		default:
			Error_Handler();
		}
	} else {
		printf("Error: Failed to decode protobuf message\n");
	}
}

void message_isr_data_recv(uint8_t *this_buffer, uint32_t this_length) {
    // Setup state.
    slip_packet_buffer = this_buffer;
    slip_packet_buffer_length = this_length;
    slip_packet_buffer_index = 0;
    
    // Decode SLIP packet.
    while (true) {
        if (slip_packet_buffer_index >= slip_packet_buffer_length) {
            // No more data to process.
            return;
        }

        if (payload_buffer_index >= MAX_BUFFER_SIZE) {
            // Payload buffer overrun, discard packet.
            payload_buffer_index = 0;
            slip_packet_continuation = false;
            printf("Error: Payload buffer overrun, discarding packet\r\n");
            return;
        }

        recv_char_call_count = 0;
        slips_context.check_start = !slip_packet_continuation;
        if (slips_recv_packet(&slips_context)) {
            // Now have a fully decoded packet.
            slip_packet_continuation = false;
            break;
        }
        
        // Payload packet not decoded (fully?) due to invalid SLIP data / buffer overrun etc.
        slip_packet_continuation = recv_char_call_count > 1;
        printf("Error: Invalid or partial packet received\r\n");
    }

    // Check length of payload.
    if (payload_buffer_index < 2) {
        goto out;
    }

    uint16_t payload_length = *(uint16_t *)(&payload_buffer[0]);
    if (payload_length != payload_buffer_index) {
        goto out;
    }

    message_isr_handle_protobuf_message(&payload_buffer[2], payload_length - 2);

out:
    // Reset state.
    payload_buffer_index = 0;
}
