#ifndef IEEE802154_H
#define IEEE802154_H

#include	<stdint.h>

typedef struct ieee802154_packet_t {
	uint8_t		channel;
	uint16_t	device;
	uint8_t		length;
	uint8_t		lqi;
	uint8_t		*data;
	char		src_addr[24];
	char		dst_addr[24];
	char		pan_addr[24];
	uint8_t		seq;
	char		desc[256];
} IEEE802154$_PDU;

void ieee802154_decode(uint8_t channel, uint8_t length,
		       uint8_t *data, int8_t rssi, uint8_t device_id,
		       struct ieee802154_packet_t *packet);

#endif /* IEEE802154_H */
