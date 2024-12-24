/*
 *  FACILITY: A yet another sniffer for ZigBee network
 *
 *  DESCRIPTION: A little programm to sniffing ZigBee trafic by using cc2531 adapter
 *
 *  AUTHOR: Owen Fraser-Green <owen@fraser-green.com>
 *
 *  CREATION DATE: ??-???-????
 *
 *  MODIFICATION HISTORY:
 *
 *	24-DEC-2024	RRL	A some refactoring to be more readable,
 *				remove "zep" part of functionality.
 *
 */



#include	<stdio.h>
#include	<stdlib.h>
#include	<stdint.h>
#include	<string.h>

#include	"utility_routines.h"
#include	"cc2531.h"
#include	"ieee802154.h"



int	g_trace = 1;


static int  s_do_sniff(unsigned char channel)
{
struct cc2531_t *l_cc2531;
struct cc2531_frame_t l_frame;
struct ieee802154_packet_t l_packet;
int	l_rc;

	$LOG(STS$K_INFO, "cc2531-sniffer initializing.");

	if ( !(1 & cc2531_create(&l_cc2531)) )
		return	cc2531_free(l_cc2531), $LOG(STS$K_ERROR, "cc2531_create() failed.");


	if ( !(1 & cc2531_set_channel(l_cc2531, channel)) )
		return	cc2531_free(l_cc2531), $LOG(STS$K_ERROR, "cc2531_set_channel() failed.");

	if ( !(1 & cc2531_start_capture(l_cc2531)) )
		return	cc2531_free(l_cc2531), $LOG(STS$K_ERROR, "cc2531_start_capture() failed.");


	for  (l_rc = STS$K_ERROR; ; )
		{
		if ( !(1 & (l_rc = cc2531_get_next_packet(l_cc2531, &l_frame))) )
			{
			l_rc = $LOG(STS$K_ERROR, "cc2531_get_next_packet() failed.");
			break;
			}

		ieee802154_decode(channel, l_frame.length, l_frame.data, l_frame.rssi, l_frame.device_id, &l_packet);

		$IFTRACE(g_trace, "src: <%s>, dst: <%s>, pan: <%6s>, seq: %3d --- %s", l_packet.src_addr, l_packet.dst_addr,
			l_packet.pan_addr, l_packet.seq, l_packet.desc);

		$DUMPHEX(l_packet.data, l_packet.length);
		}

	/* Clean up */
	cc2531_free(l_cc2531);

	return l_rc;
}

void print_usage()
{
	$LOG(STS$K_INFO, "Usage: cc22531-sniffer <CHANNEL>");
}


int	main(int argc, char* argv[])
{
unsigned char l_channel = 0;

	l_channel = atoi(argv[1]);

	if ( (l_channel < 11) || (l_channel > 26) )
		return $LOG(STS$K_ERROR, "Channel out of range. Must be from 11 to 26.");


	if (l_channel == 0 )
		{
		print_usage();
		return(EXIT_FAILURE);
		}

	s_do_sniff(l_channel );
}
