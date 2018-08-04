/**
 * \file
 *          To implement blacklisting feature for  (6TiSCH) Operation Sublayer (6top) Protocol (6P)
 *
 *
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 *         Shalu R <shalur@cdac.in>
 *         Lijo Thomas <lijo@cdac.in>
 */
#include "contiki-lib.h"

#include "lib/assert.h"
#include "net/mac/tsch/tsch.h"
#include "net/mac/tsch/sixtop/sixtop.h"
#include "net/mac/tsch/sixtop/sixp.h"
#include "net/mac/tsch/sixtop/sixp-nbr.h"
#include "net/mac/tsch/sixtop/sixp-pkt.h"
#include "net/mac/tsch/sixtop/sixp-trans.h"

#include "os/lib/list.h"
#include "contiki.h"



static const uint16_t slotframe_handle = 0;



bool listChanged = false;



/* add blacklist entries */
void addBlacklistEntry(blacklistEntry_t *element){
		list_add(BLACKlist_ENTRY, element);
	
}

/* initialize the private blacklist*/
void initPrivateBlacklist(){
	listChanged= false;
	list_init(BLACKlist_ENTRY);
	list_init(NBR_BLACKlist);
	privateBlacklist.options = 0x7;
	privateBlacklist.version = 0x1;
	privateBlacklist.blacklistEntries = NULL;
}

/* update the blacklist */
void updatePrivateBlacklist(uint8_t options, uint16_t version){
		 listChanged = false;
		 privateBlacklist = (privateBlacklist_t){.options = options, .version = version, .blacklistEntries = list_head(BLACKlist_ENTRY)};
	
}

void printPrivateBlacklist(){
	privateBlacklist.blacklistEntries = list_head(BLACKlist_ENTRY);
	printf("Blacklist Length: %d\n", list_length(BLACKlist_ENTRY));
	while(privateBlacklist.blacklistEntries !=NULL){
		printf("Blacklist Channel %d Start %u End %u Period %d: \n", privateBlacklist.blacklistEntries->channel_num,privateBlacklist.blacklistEntries->start,privateBlacklist.blacklistEntries->end,privateBlacklist.blacklistEntries->period);
		privateBlacklist.blacklistEntries = list_item_next(privateBlacklist.blacklistEntries);
	}
}

void printNeighborBlacklist(privateBlacklist_t *p){
	while(p->blacklistEntries !=NULL){
		printf("Neighbor Blacklist Channels are: %d\n",p->blacklistEntries->channel_num);
		p->blacklistEntries = list_item_next(p->blacklistEntries);
	}
}

/* set the contents of blacklist to an array for sending */
void setBuff(){
	memset(blacklist_req_storage, 0, sizeof(blacklist_req_storage));
	
	privateBlacklist.blacklistEntries = (blacklistEntry_t *)list_head(BLACKlist_ENTRY);


	unsigned char buff[4 + ((sizeof(blacklistEntry_t)-10)*NUM_BL_ENTRIES)]; //4 bytes is the size of version and options followed by the size of blacklist entries minus 10 which is the *next pointer not used here
	printf("Size of buff %d, %d\n",sizeof(buff),sizeof(blacklist_req_storage));

	/* copy the private blacklist structure to buff array */
	memcpy(buff, (char*)&privateBlacklist.options,sizeof(privateBlacklist.options));
	memcpy(buff+sizeof(privateBlacklist.options),  (char*)&privateBlacklist.version,sizeof(privateBlacklist.version));
	int i = 0;
	while(privateBlacklist.blacklistEntries !=NULL){
		memcpy(buff+sizeof(privateBlacklist.version)+sizeof(privateBlacklist.options)+i,  (char *)&privateBlacklist.blacklistEntries->channel_num ,sizeof(privateBlacklist.blacklistEntries->channel_num));
		memcpy(buff+sizeof(privateBlacklist.version)+sizeof(privateBlacklist.options)+sizeof(privateBlacklist.blacklistEntries->channel_num)+i,  (char*)&privateBlacklist.blacklistEntries->start,sizeof(privateBlacklist.blacklistEntries->start));
		memcpy(buff+sizeof(privateBlacklist.version)+sizeof(privateBlacklist.options)+sizeof(privateBlacklist.blacklistEntries->channel_num)+sizeof(privateBlacklist.blacklistEntries->start)+i,  (char*)&privateBlacklist.blacklistEntries->end,sizeof			(privateBlacklist.blacklistEntries->end));
		memcpy(buff+sizeof(privateBlacklist.version)+sizeof(privateBlacklist.options)+sizeof(privateBlacklist.blacklistEntries->channel_num)+sizeof(privateBlacklist.blacklistEntries->start)+sizeof(privateBlacklist.blacklistEntries->end)+i,  (char*)&privateBlacklist.blacklistEntries->period,sizeof(privateBlacklist.blacklistEntries->period));
		privateBlacklist.blacklistEntries = list_item_next(privateBlacklist.blacklistEntries);
		i+=10; //Start, End, Period is 10 bytes. we should add it to the size for each entry in the buff
	
	}
	
	/* print contents of buff */
	/*
	i = 0;
	printf("Contents of buff: ");
	while(i<sizeof(buff)){
	printf("%u, ",buff[i]);
	i++;
	}
	printf("\n");
	*/
	/* set the buff to blacklist_req_storage for sending to neighbor */
	 if(sixp_pkt_set_cell_list(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_ADD,
                            (const uint8_t *)buff,
                            sizeof(buff), 0,
                            blacklist_req_storage, sizeof(blacklist_req_storage))!=0){
		printf("Error sending Blacklist\n");	
		return -1;
	}
	
	/* print contents of blacklist_req_storage. first 4 bytes are the cell list options (TX, RX, SH, TK) and number of cells */
	/*
	i = 4;
	printf("Contents of black list request storage: ");
	while(i<sizeof(blacklist_req_storage)){
	printf("%u, ",blacklist_req_storage[i]);
	i++;
	}
	printf("\n");
	*/
}
/* send the blacklist */
int sendPrivateBlacklist(linkaddr_t *peer_addr){

	struct tsch_slotframe *sf =
   	 tsch_schedule_get_slotframe_by_handle(slotframe_handle);

  	assert(peer_addr != NULL && sf != NULL);

	/* send the blacklist_req_storage to neighbor */
	if(peer_addr != NULL){
	sixp_output(SIXP_PKT_TYPE_REQUEST, (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_SIGNAL,
              SF_SIMPLE_SFID,
              blacklist_req_storage, sizeof(blacklist_req_storage), peer_addr,
              NULL, NULL, 0);
	}else{
		printf("Null Address");
	}

	return 0;
}

/* Blacklist input */

void blacklistInput(const uint8_t *body, uint16_t body_len, const linkaddr_t *peer_addr){

	uint16_t reqLen, i;
	

	
	/* receive the blacklist */
	const uint8_t *buff;
	if( sixp_pkt_get_cell_list(SIXP_PKT_TYPE_REQUEST,
                            (sixp_pkt_code_t)(uint8_t)SIXP_PKT_CMD_SIGNAL,
                            &buff, &reqLen,
                            body, body_len) != 0){
		printf("Error receiving blacklist");
		return;
	}
	
	
	/* print received contents */
	/*
	
	while(i < reqLen){
		printf("%d, ", buff[i]);
		i++;
	}
	printf("\n");
	*/
	
	/* 
		process the received black list here. 
		Neighbor blacklist mangaement should be done here. 
			a) Make a new nbrBlacklsit for each neihgbor. 
			b) store the blacklist entries received from them 
			c) and finally blacklist  
	*/ 

	nbrCounter++;
	if(nbrCounter >= 4)  {
		nbrCounter = 4;
		printf("sixp-blacklist: Can not receive neighbor blacklist, Maximum neighborlist exceeded. ");
		return;
	}
	
	memcpy((char*)&receivedBlacklist[nbrCounter].nbrAddress,(char*)peer_addr,sizeof(receivedBlacklist[nbrCounter].nbrAddress));
	memcpy((char*)&receivedBlacklist[nbrCounter].options,(char*)buff,sizeof(receivedBlacklist[nbrCounter].options));
	memcpy((char*)&receivedBlacklist[nbrCounter].version, (char*)buff+sizeof(receivedBlacklist[nbrCounter].options), sizeof(receivedBlacklist[nbrCounter].version));
	

	i = 0;
	int count = 0;
	/* channel_num, start, end, period are arrays with size NUM_BL_ENTRIES = 2. 
	   change the magic number 2 to CONSTANT NUM_BL_ENTRIES
	   divide by 2 to get the size of each individual element
	*/
	uint8_t chSize,startSize,endSize,periodSize;
	chSize = sizeof(receivedBlacklist[nbrCounter].channel_num) / NUM_BL_ENTRIES;
	startSize = sizeof(receivedBlacklist[nbrCounter].start) / NUM_BL_ENTRIES;
	endSize = sizeof(receivedBlacklist[nbrCounter].end) / NUM_BL_ENTRIES;
	periodSize = sizeof(receivedBlacklist[nbrCounter].period) / NUM_BL_ENTRIES;
	while(i<2){
		memcpy((char*)&receivedBlacklist[nbrCounter].channel_num[i], (char*)buff+sizeof(receivedBlacklist[nbrCounter].options)+sizeof(receivedBlacklist[nbrCounter].version) + count, chSize);
		memcpy((char*)&receivedBlacklist[nbrCounter].start[i], (char*)buff+sizeof(receivedBlacklist[nbrCounter].options)+sizeof(receivedBlacklist[nbrCounter].version)+chSize + count, startSize);
		memcpy((char*)&receivedBlacklist[nbrCounter].end[i], (char*)buff+sizeof(receivedBlacklist[nbrCounter].options)+sizeof(receivedBlacklist[nbrCounter].version)+chSize+startSize + count, endSize);
		memcpy((char*)&receivedBlacklist[nbrCounter].period[i], (char*)buff+sizeof(receivedBlacklist[nbrCounter].options)+sizeof(receivedBlacklist[nbrCounter].version) + chSize + startSize + endSize + count, periodSize);
		count += 10;
		i++;
	}
	
				
	printf("Blacklist received successfully with contents: \n");
	printf("Blacklsit Neighbor Address: %d\n", receivedBlacklist[nbrCounter].nbrAddress.u8[1]);	
	printf("Blacklist Options: %d,\n", receivedBlacklist[nbrCounter].options);
	printf("Blacklist Version: %d,\n", receivedBlacklist[nbrCounter].version);
	i = 0;
	while(i<2){
		printf("Neighbor Blacklist Contents %u, %u, %u, %u \n", receivedBlacklist[nbrCounter].channel_num[i],receivedBlacklist[nbrCounter].start[i],receivedBlacklist[nbrCounter].end[i],receivedBlacklist[nbrCounter].period[i]);
		i++;	
	}
	
	/* Add the received blacklist to the neighbor blacklist */
	list_add(NBR_BLACKlist, &receivedBlacklist[nbrCounter] );
	printf("Neighbor Blacklist len: %d\n", list_length(NBR_BLACKlist));
	
}

uint8_t checkPrivatBlacklist(uint8_t ch){
	privateBlacklist.blacklistEntries = (blacklistEntry_t *)list_head(BLACKlist_ENTRY);
	uint8_t block = 0;
	/* check the private blacklist */ 
	while(privateBlacklist.blacklistEntries !=NULL){
		clock_time_t currentTime = clock_seconds();
		//printf("Reading Private Blacklist\n");
		//printf("ch, prvBlackCh, %d, %d\n", ch,privateBlacklist.blacklistEntries->channel_num);
		if(privateBlacklist.blacklistEntries->channel_num == ch) {
			if((privateBlacklist.blacklistEntries->start<=currentTime)&&(currentTime<=privateBlacklist.blacklistEntries->end)){
				//printf("Timer Holds\n");
				//printf("Start, End, CurrentTime, %u, %u, %u\n", privateBlacklist.blacklistEntries->start,privateBlacklist.blacklistEntries->end,currentTime);
				block = 1; 
				return block;
			}
			else{
				//printf("Timer expired or not started yet \n");
				//printf("Start, End, CurrentTime, %u, %u, %u\n", privateBlacklist.blacklistEntries->start,privateBlacklist.blacklistEntries->end,currentTime);
				block = 0; 
				return block;
			}
			
		}
		privateBlacklist.blacklistEntries = list_item_next(privateBlacklist.blacklistEntries);
	}
	return block;
}

uint8_t checkNbrBlacklist(uint8_t ch){
	/* check the neighbor blacklist */
	uint8_t block = 0; 
	nbrBlacklist_t *p;
	p = (nbrBlacklist_t *)list_head(NBR_BLACKlist);
	while(p != NULL){
		//printf("Reading Nbr Blacklist(from cehckNbrBlacklist)\n");
		int i = 0;
		while(i<sizeof(p->channel_num)){
			clock_time_t currentTime = clock_seconds();
			//printf("ch, nbrCh, %d, %d\n", ch, p->channel_num[i]);
			if(p->channel_num[i] == ch) { 
				if((p->start[i]<=currentTime)&&(currentTime<=p->end[i])){
					//printf("Nbr Timer Holds\n");
					//printf("Start, End, CurrentTime, %u, %u, %u\n", p->start[i],p->end[i],currentTime);
					block = 1; 
					return block;
				}
				else{
					//printf("Nbr Timer expired or not started yet \n");
					//printf("Start, End, CurrentTime, %u, %u, %u\n", p->start[i],p->end[i],currentTime);
					block = 0; 
					return block;
				}
			}
			i++;
		}
		p = list_item_next(p);
	}
	return block;
}



