

#include <stdio.h>
#include "net/ppp/hdr.h"
#include "byteorder.h"

#define BY2BINPT "0x%d%d%d%d%d%d%d%d"
#define BY2BIN(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0) 

void ppp_hdr_print(ppp_hdr_t *hdr)
{

	//Print address
	//printf("Address: %" BY2BINPT "\n", BY2BIN(ppp_hdr_get_address(hdr)));

	//Print protocol
	//printf("Protocol: %" PRIu16 "\n", ppp_hdr_get_protocol(hdr));

	//Print FCS
	network_uint32_t fcs;
	fcs = ppp_hdr_get_fcs(hdr);

	//printf("FCS: %" PRIu16 ", %" PRIu16 "\n",fcs[0], fcs[1]);
}
