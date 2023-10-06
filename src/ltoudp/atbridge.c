/*
   GSPLUS - Advanced Apple IIGS Emulator Environment
   Based on the KEGS emulator written by Kent Dickey
   See COPYRIGHT.txt for Copyright information
   See LICENSE.txt for license (GPL v2)
 */

/** This module is the "heart" of the bridge and provides the connection between the ELAP and LLAP ports. **/

#include <stdbool.h>
#include "../defc.h"
#include <stddef.h>
#include <time.h>
#include "atbridge.h"
#include "port.h"
#include "llap.h"
#include "LToUDP.h"
#include <assert.h>

#ifdef WIN32
#include <winsock2.h>
#elif __linux__
#include <netinet/in.h>
#endif

extern struct packet_port_t elap_port;

static bool diagnostics = false;

static struct at_addr_t local_address = {0, 0};

static const at_network_t NET_STARTUP_LOW = 0xFF00;
static const at_network_t NET_STARTUP_HIGH = 0xFFFE;
static const at_node_t NODE_STARTUP_LOW = 0x01;
static const at_node_t NODE_STARTUP_HIGH = 0xFE;

bool atbridge_init()
{
  // If the GS reboots, we may try to reinitialize the bridge.  If this is the case, keep the old address and AMT.
  if (local_address.network == 0)
  {
    // Obtain a provisional node address and startup range network.
    //
    // This isn't correct for an extended network (like ELAP) but works adequately on small networks.
    // The bridge should follow the complicated process on page 4-9 to obtain the network and node number.
    srand((unsigned int)time(0));
    local_address.network = (at_network_t)((double)rand() / RAND_MAX * (NET_STARTUP_HIGH - NET_STARTUP_LOW) + NET_STARTUP_LOW);
    local_address.node = (at_node_t)((double)rand() / RAND_MAX + (NODE_STARTUP_HIGH - NODE_STARTUP_LOW) + 0x01);

    bool udpStarted = UDP_startupExecute();
    assert(udpStarted);
    llap_init();
  }
  return true;
}

void atbridge_shutdown()
{
  llap_shutdown();
  UDP_stop();
}

void atbridge_set_diagnostics(bool enabled)
{
  diagnostics = enabled;
}

bool atbridge_get_diagnostics()
{
  return diagnostics;
}

void atbridge_printf(const char *fmt, ...)
{
  if (atbridge_get_diagnostics())
  {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
  }
}

const struct at_addr_t *atbridge_get_addr()
{
  return &local_address;
}

const at_network_t atbridge_get_net()
{
  return local_address.network;
}

const at_node_t atbridge_get_node()
{
  return local_address.node;
}

void atbridge_set_net(at_network_t net)
{
  local_address.network = net;
}

void atbridge_set_node(at_node_t node)
{
  local_address.node = node;
}

void atbridge_process()
{
  struct packet_t *packet = UDP_dequeueIn();
  if (packet)
  {
    llap_enqueue_out(packet);
  }
  packet = llap_dequeue_in();
  if (packet)
  {
    printf("Attempting to send data: ");
    for (int i = 0; i<packet->size; i++) printf("%02X", packet->data[i]);
    printf("\n");
    UDP_enqueueOut(packet);
  }
}