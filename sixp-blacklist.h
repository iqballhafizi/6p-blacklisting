/**
 * \file
 *         To implement blacklisting feature for  (6TiSCH) Operation Sublayer (6top) Protocol (6P)
 *
 * \author
 *         Ahmad Iqbal Hafizi <ahmad.hafizi@tuhh.de>
 */
#include "os/net/linkaddr.h"

/**

6p Black list header

*/


//const uint8_t NUM_BL_ENTRIES = 2; //Total number of blacklist entries
//const uint8_t BLACKlist_UPDATE_TIMEOUT = 2; //The blacklist update should be propagated to neighbors within this time. if not, explicitly send the update using 6P SIGNAL message.
//const uint8_t CHANNEL_SWAP_OFFSET = 0; //an offset to swap the channel if the blacklisted channel is selected for transmission
//const uint8_t BLACKlist_LIFETIME = 5; 

/**

Structure to hold the blacklist information

@datamembers
	name: a short description
	channel_number: the channel to block
	timeout: the point in time at which this entry becomes invalid
*/

//Size of this is 20 Bytes
typedef struct blacklistEntry{
	struct blacklistEntry *next;
	uint8_t channel_num;
	clock_time_t start;
	clock_time_t end;
	uint8_t period;
	
}blacklistEntry_t;

//Size of this is 24 bytes
typedef struct privateBlacklist{
	uint16_t options;
	uint16_t version;
	blacklistEntry_t *blacklistEntries;
}privateBlacklist_t;

typedef struct nbrBlacklist{
	struct nbrBlacklist *next;
	linkaddr_t nbrAddress;
	uint16_t options;
	uint16_t version;
	uint8_t channel_num[2];
	clock_time_t start[2];
	clock_time_t end[2];
	uint8_t period[2];
}nbrBlacklist_t;

#define SF_SIMPLE_SFID       0xf0
/*list to store blacklist entries */
LIST(BLACKlist_ENTRY);
LIST(NBR_BLACKlist);


/* Structure for private blacklist, nbrBlacklist, and private blacklist entries */
privateBlacklist_t privateBlacklist;
blacklistEntry_t element1,element2;
nbrBlacklist_t receivedBlacklist;
static uint8_t blacklist_req_storage[8 + ((sizeof(blacklistEntry_t)-10) * 2)]; //4 byte is the begining + 4 bytes is the size of version and options followed by the size of blacklist entries - 10 which is the *next pointer not used here

/* functions */

void addBlacklistEntry(blacklistEntry_t *element);
void initPrivateBlacklist();
void updatePrivateBlacklist(uint8_t options, uint16_t version);
void printPrivateBlacklist();
void printNeighborBlacklist(privateBlacklist_t *p);
void setBuff();
int sendPrivateBlacklist(linkaddr_t *peer_addr);
void blacklistInput(const uint8_t *body, uint16_t body_len, const linkaddr_t *peer_addr);
uint8_t checkPrivatBlacklist(uint8_t ch);
uint8_t checkNbrBlacklist(uint8_t ch);

