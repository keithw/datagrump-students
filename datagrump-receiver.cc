#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include <queue>
#include <cassert>
#include <utility>
#include <stdio.h>
#include "timestamp.hh"
#include "socket.hh"

using namespace Network;
using namespace std;

int main( int argc, char *argv[] )
{
  /* check arguments */
  if ( argc != 2 ) {
    fprintf( stderr, "Usage: %s PORT\n", argv[ 0 ] );
    exit( 1 );
  }

  try {
    /* Create UDP socket for incoming datagrams. */
    Network::Socket sock;

    /* Listen on UDP port. */
    sock.bind( Address( "0" /* all IP addresses */,
			argv[ 1 ] /* port */ ) );

    fprintf( stderr, "Listening on port %s...\n", argv[ 1 ] );
    fprintf( stderr, "Start On %d \n",timestamp());

    /* Loop */
    uint64_t sequence_number = 0;
    while ( 1 ) {
      Packet received_packet = sock.recv();
      fprintf( stderr, "Recv on %d\n",timestamp());

      /* Send back acknowledgment */
      Packet ack( received_packet.addr(), sequence_number++, received_packet );
      sock.send( ack );
    }
  } catch ( const string & exception ) {
    /* We got an exception, so quit. */
    fprintf( stderr, "Exiting on exception: %s\n", exception.c_str() );
    exit( 1 );
  }

  return 0;
}
