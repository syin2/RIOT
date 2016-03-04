
#ifndef HDLC_HDR_H
#define HDLC_HDR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


uint16_t hdlc_fcs16(uint16_t, uint8_t*, int);
uint32_t hdlc_fcs32(uint32_t, uint8_t*, int);

#ifdef __cplusplus
}
#endif

#endif
