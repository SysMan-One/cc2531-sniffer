#include	<stdlib.h>
#include	<stdio.h>
#include	<time.h>
#include	<linux/types.h>

#include	"libusb.h"
#include	"cc2531.h"
#include	"utility_routines.h"

#define CC2531_INTERFACE	0
#define CC2531_TIMEOUT		1000
#define CC2531_BUFFER_SIZE	256


enum _cc2531_request {
  CC2531_REQUEST_CONTROL = 9
};


struct cc2531_t {
	libusb_device_handle *dh;
	unsigned char buf[CC2531_BUFFER_SIZE];
	uint16_t device_id;
};


struct  __attribute__ ((__packed__)) cc2531_usb_header_t {
	uint8_t	info;
	__be16	length;
	__be32	timestamp;
};


extern int g_trace;



static int s_cc2531_handle_transfer(struct cc2531_t *a_cc2531, int a_rc)
{
	if (a_rc >= 0)
		return	$IFTRACE(g_trace, "USB transfer: %d bytes.", a_rc), STS$K_SUCCESS;


	switch(a_rc)
		{
		case LIBUSB_ERROR_TIMEOUT:
			return $LOG(STS$K_ERROR, "USB transfer timeout.");

		case LIBUSB_ERROR_PIPE:
			return $LOG(STS$K_ERROR, "Control request not supported by USB device.");

		case LIBUSB_ERROR_NO_DEVICE:
			return $LOG(STS$K_ERROR, "USB device has been disconnected.");

		default:
			return $LOG(STS$K_ERROR, "USB error during transfer: ", a_rc);
		}
}

static inline int s_cc2531_set_config(struct cc2531_t *a_cc2531, unsigned short c)
{
	int l_rc = libusb_control_transfer(a_cc2531->dh, LIBUSB_ENDPOINT_OUT, CC2531_REQUEST_CONTROL, c, 0, NULL, 0, CC2531_TIMEOUT);
	return s_cc2531_handle_transfer(a_cc2531, l_rc);
}

static inline int s_cc2531_get_ctrl(struct cc2531_t *a_cc2531, unsigned short c, unsigned short length)
{
	int l_rc = libusb_control_transfer(a_cc2531->dh, 0xC0, c, 0, 0, a_cc2531->buf, length, CC2531_TIMEOUT);
	return s_cc2531_handle_transfer(a_cc2531, l_rc);
}

static inline int s_cc2531_set_ctrl(struct cc2531_t *cc2531, unsigned short c, unsigned short length, unsigned short index)
{
	int l_rc = libusb_control_transfer(cc2531->dh, 0x40, c, 0, index, cc2531->buf, length, CC2531_TIMEOUT);
	return s_cc2531_handle_transfer(cc2531, l_rc);
}





static int s_cc2531_wait_for_198(struct cc2531_t *a_cc2531)
{
struct timespec l_ts = {.tv_sec = 0, l_ts.tv_nsec = 62400000};
int l_cnt = 1;

	s_cc2531_get_ctrl(a_cc2531, 198, 1);

	while (a_cc2531->buf[0] != 4)
		{
		nanosleep(&l_ts, NULL);
		s_cc2531_get_ctrl(a_cc2531, 198, 1);

		if (++l_cnt > 20)
			return $LOG(STS$K_ERROR, "Didn't receive acknowledgment from CC2531 dongle.");
		}

	return STS$K_SUCCESS;
}


int	cc2531_create(struct cc2531_t ** a_cc2531)
{
libusb_device *l_dev = NULL;
libusb_device_handle *l_dh = NULL;
libusb_device **l_dev_list = NULL;
struct libusb_device_descriptor desc;
struct cc2531_t *l_cc2531 = NULL;
unsigned char l_desc_manufacturer[256];
unsigned char l_desc_product[256];
unsigned char l_desc_serial[256];
int	l_num_devices = 0, l_rc, l_cnt;

	if (LIBUSB_SUCCESS != (l_rc = libusb_init(NULL)) )
		return $LOG(STS$K_ERROR, "Failed to initialize libusb: %d", l_rc);

	l_num_devices = libusb_get_device_list(NULL, &l_dev_list);

	for (l_cnt = 0; l_cnt < l_num_devices; l_cnt++)
		{
		l_dev = l_dev_list[l_cnt];

		if ( LIBUSB_SUCCESS != (l_rc = libusb_get_device_descriptor(l_dev, &desc)) )
			return $LOG(STS$K_ERROR, "Failed to get the USB device descriptor: %d", l_rc);

		$IFTRACE(g_trace, "idVendor: %#x, idProduct: %#x", desc.idVendor, desc.idProduct);


		if ( (desc.idVendor == USB_VID) && (desc.idProduct == USB_PID) )
			break;
		}

	if (l_cnt >= l_num_devices)
		return $LOG(STS$K_ERROR, "Failed to find CC2531 (idVendor: %#x, idProduct: %#x", USB_VID, USB_PID);




	switch (libusb_open(l_dev, &l_dh))
		{
		case LIBUSB_SUCCESS:
			break;

		case LIBUSB_ERROR_NO_MEM:
			return $LOG(STS$K_ERROR, "No memeory to open USB device.");

		case LIBUSB_ERROR_ACCESS:
			return $LOG(STS$K_ERROR, "Insufficient privileges to open USB device.");

		case LIBUSB_ERROR_NO_DEVICE:
			return $LOG(STS$K_ERROR, "Device has been disconnected.");

		default:
			return $LOG(STS$K_ERROR, "Error opening USB device: %d", l_rc);
		}




	if ( 0 > (l_rc = libusb_get_string_descriptor_ascii(l_dh, desc.iManufacturer, l_desc_manufacturer, 256)) )
		return $LOG(STS$K_ERROR, "Failed to get the manufacturer: %d", l_rc);

	if ( 0 > (l_rc = libusb_get_string_descriptor_ascii(l_dh, desc.iProduct, l_desc_product, 256)) )
		return $LOG(STS$K_ERROR, "Failed to get the product: %d", l_rc);

	$LOG(STS$K_INFO, "Using CC2531 on USB bus %03d device %03d. Manufacturer: \"%s\" Product: \"%s\" Serial: %04x",
		libusb_get_bus_number(l_dev),
		libusb_get_device_address(l_dev),
		l_desc_manufacturer,
		l_desc_product,
		desc.bcdDevice);

	/* Try and claim the device */
	l_rc = libusb_claim_interface(l_dh, CC2531_INTERFACE);

	switch (l_rc)
		{
		case LIBUSB_SUCCESS:
			break;

		case LIBUSB_ERROR_BUSY:
			return $LOG(STS$K_ERROR, "The USB device is in use by another program or driver.");

		case LIBUSB_ERROR_NO_DEVICE:
			return $LOG(STS$K_ERROR, "The USB device has been disconnected.");

		case LIBUSB_ERROR_NOT_FOUND:
			return $LOG(STS$K_ERROR, "The USB device was not found.");

		default:
			return $LOG(STS$K_ERROR, "Failed to claim USB device: %d", l_rc);
		}

	l_cc2531 = malloc(sizeof(struct cc2531_t));
	l_cc2531->dh = l_dh;
	l_cc2531->device_id = desc.bcdDevice;

	if ( !(1 & s_cc2531_set_config(l_cc2531, 0)) )
		return	free(l_cc2531), libusb_release_interface(l_dh, CC2531_INTERFACE),
				$LOG(STS$K_ERROR, "s_cc2531_set_config() failed");

	if ( !(1 & s_cc2531_get_ctrl(l_cc2531, 192, 256)) )
		return	free(l_cc2531), libusb_release_interface(l_dh, CC2531_INTERFACE),
				$LOG(STS$K_ERROR, "s_cc2531_get_ctrl() failed");

	*a_cc2531 = l_cc2531;

	return	STS$K_SUCCESS;
}

int cc2531_set_channel(struct cc2531_t *cc2531, unsigned char channel)
{
	$LOG(STS$K_INFO, "Setting CC2531 to channel %d", channel);

	if ( !(1 & s_cc2531_set_config(cc2531, 1)) )
		return	STS$K_ERROR;
	if ( !(1 & s_cc2531_set_ctrl(cc2531, 197, 0, 4)) )
		return	STS$K_ERROR;
	if ( !(1 & s_cc2531_wait_for_198(cc2531)) )
		return	STS$K_ERROR;
	if ( !(1 & s_cc2531_set_ctrl(cc2531, 201, 0, 0)) )
		return	STS$K_ERROR;

	cc2531->buf[0] = channel;
	if ( !(1 & s_cc2531_set_ctrl(cc2531, 210, 1, 0)) )
			return	STS$K_ERROR;

	return STS$K_SUCCESS;
}

static int	s_cc2531_read_frame(
		struct cc2531_t *a_cc2531
		)
{
int l_transferred = 0, l_rc = 0;

	l_rc = libusb_bulk_transfer(a_cc2531->dh, 0x83 , a_cc2531->buf, CC2531_BUFFER_SIZE, &l_transferred, 0);

	switch (l_rc)
		{
		case LIBUSB_SUCCESS:
			$IFTRACE(g_trace, "Bulk read %d bytes.", l_transferred);
			break;

		case LIBUSB_ERROR_TIMEOUT:
			return $LOG(STS$K_ERROR, "Timeout reading data from CC2531 dongle.");

		case LIBUSB_ERROR_PIPE:
			return $LOG(STS$K_ERROR, "CC2531 Endpoint halted.");

		case LIBUSB_ERROR_OVERFLOW:
			return $LOG(STS$K_ERROR, "CC2531 overflow.");

		case LIBUSB_ERROR_NO_DEVICE:
			return $LOG(STS$K_ERROR, "CC2531 has been disconnected.");

		default:
			return $LOG(STS$K_ERROR, "CC2531 bulk read failed: %d", l_rc);
		}

	if ( l_transferred > sizeof(struct cc2531_usb_header_t) )
		return l_transferred;

	return STS$K_SUCCESS;
}

int cc2531_start_capture(struct cc2531_t *a_cc2531)
{

	a_cc2531->buf[0] = 0;

	if ( !(1 & s_cc2531_set_ctrl(a_cc2531, 210, 1, 1)) )
		return	STS$K_ERROR;

	if ( !(1 & s_cc2531_set_ctrl(a_cc2531, 208, 0, 0)) )
		return	STS$K_ERROR;

	return STS$K_SUCCESS;
}

int cc2531_get_next_packet(
		struct cc2531_t *a_cc2531,
		struct cc2531_frame_t *a_frame
		)
{
struct cc2531_usb_header_t *l_header;

	if ( !(1 &  s_cc2531_read_frame(a_cc2531)) )
		return	STS$K_ERROR;


	l_header = (struct cc2531_usb_header_t *) a_cc2531->buf;


	/* Skip everything other than 0 frames */
	if (l_header->info != 0)
		return cc2531_get_next_packet(a_cc2531, a_frame);

	$IFTRACE(g_trace, "Received frame with info %02x and length %d and timestamp %d",
		 l_header->info, le16toh(l_header->length), le32toh(l_header->timestamp));

	a_frame->length = a_cc2531->buf[7];
	a_frame->data = &a_cc2531->buf[8];
	a_frame->rssi = a_frame->data[a_frame->length] - 73;
	a_frame->device_id = a_cc2531->device_id;

	return STS$K_SUCCESS;
}


void cc2531_free(struct cc2531_t *a_cc2531)
{
	if ( !a_cc2531 )
		return;

	libusb_release_interface(a_cc2531->dh, CC2531_INTERFACE);
	libusb_exit(NULL);
	free(a_cc2531);
}
