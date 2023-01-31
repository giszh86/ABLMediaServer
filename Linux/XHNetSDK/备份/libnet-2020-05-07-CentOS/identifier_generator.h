#ifndef _IDENTIFIER_GENERATOR_H_
#define _IDENTIFIER_GENERATOR_H_ 

#include "libnet.h"

NETHANDLE generate_identifier();
void  recycle_identifier(NETHANDLE hId);

#endif