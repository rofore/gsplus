//
//  UDP.c
//

#include "LToUDP.h"
#include "atalk.h"


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>

#define DEBUG_LTOUDP (0)

#define SWAP_UINT32(x)                                                 \
	(((x) >> 24) | (((x)&0x00FF0000) >> 8) | (((x)&0x0000FF00) << 8) | \
	 (((x)&0xFF) << 24))




int fd;
uint32_t ltodup_id;
struct sockaddr_in my_addr;
struct sockaddr_in their_addr;
struct sockaddr_in bcast_addr;
struct ip_mreq mreq;
socklen_t addr_len;

ssize_t tmpReceiveLen;
bool terminate;

static uint32_t LToUDP_ID_GLOBAL = 100UL; // Start value for every instance

void UDP_init(void)
{ /* TBD */
}

bool UDP_startupExecute(void)
{
	ltodup_id = LToUDP_ID_GLOBAL++;
	printf("Connecting with ID = %u (0x%X)\n", ltodup_id, ltodup_id);
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	int one = 1;
	int err;
	err =
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&one, sizeof(one));
	if (err)
	{
		printf("Setting of SO_REUSEADDR not successful!\n");
		return false;
	}

	err = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one));
	if (err)
	{
		printf("Setting of SO_REUSEPORT not successful!\n");
		return false;
	}

	//   setsockopt(fd, SOL_SOCKET, IP_BOUND_IF, "en0", sizeof(one));
	//   if (err) {
	//     printf("Setting of SO_BINDTODEVICE not successful!\n");
	//     return false;
	//   }

	struct timeval read_timeout;
	read_timeout.tv_sec = 0;
	read_timeout.tv_usec = 1000;
	setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

	memset((char *)&my_addr, 0, sizeof(my_addr));

	my_addr.sin_family = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;
	my_addr.sin_port = htons(LTOUDP_PORT);

	memset((char *)&bcast_addr, 0, sizeof(bcast_addr));
	bcast_addr.sin_family = AF_INET;
	bcast_addr.sin_addr.s_addr = inet_addr("239.192.76.84");
	bcast_addr.sin_port = htons(1954);

	if (bind(fd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{
		perror("bind");
		return false;
	}

	mreq.imr_multiaddr.s_addr = inet_addr("239.192.76.84");
	mreq.imr_interface.s_addr = INADDR_ANY;

	if (0 != setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *)&mreq,
						sizeof(mreq)))
	{
		perror("UDP add membership");
		return false;
	}

	addr_len = sizeof(struct sockaddr);
	terminate = false;
	return true;
}

packet_t *UDP_dequeueIn(void)
{
	uint8_t *ltoudpBuffer = calloc(1024, 1);
	uint8_t *frameBuffer = calloc(1024, 1);
	size_t bufferLength = 0UL;
	struct packet_t *nextFrame = calloc(1, sizeof(struct packet_t));
	int tmpLen = 0;
	tmpLen = recvfrom(fd, ltoudpBuffer, LTOUDP_MAX_PACKET_LEN, 0U,
					  (struct sockaddr *)&their_addr, &addr_len);

	if (tmpLen >= 4)
	{
		memcpy(frameBuffer, &ltoudpBuffer[4], tmpLen - 4);


		bufferLength = tmpLen;

		uint32_t srcLToUDPNode = SWAP_UINT32(*((uint32_t *)ltoudpBuffer));

		if (srcLToUDPNode == ltodup_id)
		{
#if (1 == DEBUG_LTOUDP)
			printf("Packet came back, ignore it!\n");
#endif
			goto free;
		}

#if (1 == DEBUG_LTOUDP)
		printf("Received packed from LToUDP sender ID %u with length %hu.\n",
			   srcLToUDPNode, packetLength);
#endif

		memset(nextFrame, 0, sizeof(packet_t));
		nextFrame->data = frameBuffer; /* LToUDP id length */
		assert(bufferLength >= 4);
		nextFrame->size = bufferLength - 4;
		nextFrame->dest.node = nextFrame->data[0];
		nextFrame->source.node = nextFrame->data[1];
		nextFrame->type = nextFrame->data[2];
		free(ltoudpBuffer);
		return nextFrame;
	}
	free:
		free(ltoudpBuffer);
		free(frameBuffer);
		free(nextFrame);
		return NULL;

}

void UDP_enqueueOut(packet_t *buffer)
{
	uint8_t bufferOut[1024];
	if (NULL != buffer)
	{
		memcpy(&bufferOut[4], buffer->data, buffer->size);
		*(uint32_t *)bufferOut = SWAP_UINT32(ltodup_id);
#if (1 == DEBUG_LTOUDP)
		printf("dispatching packet...\n");
#endif
		if (fd)
		{
			size_t i = sendto(fd, bufferOut, buffer->size + 4, 0,
							  (struct sockaddr *)&bcast_addr, addr_len);
			if (0UL > i)
			{
				perror("send");
			}
			else
			{
#if (1 == DEBUG_LTOUDP)
				printf("Sent %lu bytes successfully!\n", buffer->length);
#endif
			}
		}
		else
		{
			printf("LToUDP: Sending of %lu bytes failed!\n", buffer->size);
		}
	}
}

bool UDP_stop(void) { return true; }
