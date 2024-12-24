#include	<stdio.h>
#include	<string.h>
#include	<endian.h>

#include	"ieee802154.h"

static const char * const s_frame_types[] = {
	"Beacon",
	"Data",
	"Ack"
};


static const char * const s_cmd_types[] = {"",
	"Association Request",
	"Association Response",
	"Disassociation Notification",
	"Data Request",
	"PAN ID Conflict",
	"Orphan Notification",
	"Beacon Request",
	"Coordinator Realignment",
	"GTS Request"
};


static const char * s_broadcast = "Broadcast";
static const char * s_unknown = "Unknown";

#define ADDR_MODE_NONE 0
#define ADDR_MODE_SHORT 2
#define ADDR_MODE_EXT 3


static inline int ieee802154_fmt_address (
		char	*a_buf,
		uint8_t	a_addr_mode,
		uint8_t	*a_data
		)
{
uint16_t l_addr;

	if  ( a_addr_mode == ADDR_MODE_SHORT)
		{
		l_addr = *((uint16_t *) a_data);
		l_addr = le16toh(l_addr);

		if  (l_addr == 0xffff)
			strncpy(a_buf, s_broadcast, 24);
		else	snprintf(a_buf, 16, "0x%04X", l_addr);

		return 2;
		}


	strncpy(a_buf, s_unknown, 24);
	return 0;

}

void ieee802154_decode (
		uint8_t	a_channel,
		uint8_t	a_length,
		uint8_t *a_data,
		int8_t	a_rssi,
		uint8_t a_device_id,
	struct ieee802154_packet_t *a_packet
		)
{
uint8_t l_frame_type, l_src_addr_mode, l_dst_addr_mode;

	a_packet->channel = a_channel;
	a_packet->device = a_device_id;
	a_packet->length = a_length;
	a_packet->lqi = a_rssi;
	a_packet->data = a_data;

	/* Get the frame description */
	l_frame_type = a_data[0] & 7;

	if (l_frame_type <= 2)
		strncpy(a_packet->desc, s_frame_types[l_frame_type], 256);
	else	strncpy(a_packet->desc, s_unknown, 256);

	l_src_addr_mode = (a_data[1] >> 6) & 3;
	l_dst_addr_mode = (a_data[1] >> 2) & 3;

	a_data += 2;
	a_packet->seq = a_data++[0];

	/* PAN */
	if (l_src_addr_mode == ADDR_MODE_SHORT || l_src_addr_mode == ADDR_MODE_EXT)
		a_data += ieee802154_fmt_address(a_packet->pan_addr, ADDR_MODE_SHORT, a_data);
	else	a_data += ieee802154_fmt_address(a_packet->pan_addr, ADDR_MODE_NONE, NULL);


	/* Destination address */
	a_data += ieee802154_fmt_address(a_packet->dst_addr, l_dst_addr_mode, a_data);

	/* Source address */
	a_data += ieee802154_fmt_address(a_packet->src_addr, l_src_addr_mode, a_data);

	/* Command */
	if ( (l_frame_type == 3) && (a_data[0] <= 9))
		strncpy(a_packet->desc, s_cmd_types[a_data[0]], 256);
}

