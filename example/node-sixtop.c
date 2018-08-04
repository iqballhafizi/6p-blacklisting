/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *         To test 6P transaction on a RPL+TSCH network
 *
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 *         Shalu R <shalur@cdac.in>
 *         Lijo Thomas <lijo@cdac.in>
 */

#include "contiki.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "net/routing/routing.h"
#include "sf-simple.h"
#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "RPL Node");
AUTOSTART_PROCESSES(&node_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static int is_coordinator;
  static int added_num_of_links = 0;
  static struct etimer et;


  PROCESS_BEGIN();

  is_coordinator = 0;

#if CONTIKI_TARGET_COOJA
  is_coordinator = (node_id == 1);
#endif

  if(is_coordinator) {
    NETSTACK_ROUTING.root_start();
  }

  NETSTACK_MAC.on();
  sixtop_add_sf(&sf_simple_driver);

	/*initialize private blacklist*/
	initPrivateBlacklist();
	

	
  etimer_set(&et, CLOCK_SECOND * 30);   

  while(1) {
    PROCESS_YIELD_UNTIL(etimer_expired(&et));
    etimer_reset(&et);

	if((added_num_of_links == node_id)) {
		
	/* Add entries to Blacklist */
		element1  = (blacklistEntry_t) { .next = NULL, .channel_num = 15, .start = clock_seconds(), .end = clock_seconds()+60, .period = 60};
		element2  = (blacklistEntry_t) { .next = NULL, .channel_num = 20, .start = clock_seconds() + 60, .end = clock_seconds() + 90, .period = 60};
		addBlacklistEntry(&element1);
		addBlacklistEntry(&element2);
	/*Print Blacklist Entries */
		printPrivateBlacklist();
	/* Copy contents of the blacklist to a buffer for sending */
		setBuff();
	//sendPrivateBlacklist(&n->addr);
		sendBlacklistToAllNbrs();
      }
      added_num_of_links++;
  }

  PROCESS_END();
}
