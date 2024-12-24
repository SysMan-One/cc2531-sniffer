#ifndef CC2531_H
#define CC2531_H

#define USB_VID 0x0451
#define USB_PID 0x16ae



typedef struct cc2531_frame_t {
	unsigned char length;
	unsigned char *data;
	char rssi;
	uint16_t device_id;
} CC2531$_FRAME;


struct cc2531_t;

int	cc2531_create(struct cc2531_t **);
int	cc2531_set_channel(struct cc2531_t *, unsigned char channel);
int	cc2531_start_capture(struct cc2531_t *);
int	cc2531_get_next_packet(struct cc2531_t *, struct cc2531_frame_t *);
void	cc2531_free(struct cc2531_t *);

#endif /* CC2531_H */

