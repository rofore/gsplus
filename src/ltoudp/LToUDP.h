//
//  UDP.h
//

#ifndef UDP_h
#define UDP_h

#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "port.h"

#define LTOUDP_MAX_PACKET_LEN ( 606 )
#define LTOUDP_PORT ( 1954 )

#define LTOUDP_LLAP_OFFSET ( 4UL )

void UDP_init(void);
bool UDP_startupExecute(void);
packet_t* UDP_dequeueIn(void);
bool UDP_stop(void);
void UDP_enqueueOut(packet_t* buffer);

#endif /* UDP_h */
