#ifndef _RTP_PACKET_DEPACKET_COMMON_H_
#define _RTP_PACKET_DEPACKET_COMMON_H_

#include <stdint.h>

uint32_t generate_identifier();

void recycle_identifier(uint32_t id);

#endif 