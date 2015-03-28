/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 *
 * A web server that just shows one page saying "OK, works great!"
 *
 * http://tuxgraphics.org/electronics/
 * Chip type           : Atmega88/168/328/644 with ENC28J60
 *********************************************/
 #if 0
#include "hal.h"
#include <stdlib.h>
#include <string.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "dhcp_client.h"
#include "timeout.h"


// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
static uint8_t mymac[6] = {0x00, 0x53,0x4C,0x46,0x07,0x01};
uint8_t netmask[4];

// My own IP (DHCP will provide a value for it):
uint8_t myip[4]={0,0,0,0};
// Default gateway (DHCP will provide a value for it):
uint8_t gwip[4]={0,0,0,0};
#define TRANS_NUM_GWMAC 1
uint8_t gwmac[6];
#define TRANS_NUM_WEBMAC 2
uint8_t server_gwmac[6];
//static uint8_t myip[4] = {192,168,0,130}; // aka http://10.0.0.29/
//static uint8_t myip[4] = {10,10,4,29};
//static uint8_t myip[4] = {192,168,2,20};

// listen port for www
#define MYWWWPORT 80

#define BUFFER_SIZE 500
static uint8_t buf[BUFFER_SIZE+1];

uint16_t http200ok(void)
{
        return(fill_tcp_data_p(buf,0,"HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
}

// the __attribute__((unused)) is a gcc compiler directive to avoid warnings about unsed variables.
void arpresolver_result_callback(uint8_t *ip __attribute__((unused)),uint8_t reference_number,uint8_t *mac){
	uint8_t i=0;
	if (reference_number==TRANS_NUM_GWMAC){
		// copy mac address over:
		while(i<6){gwmac[i]=mac[i];i++;}
	}
	if (reference_number==TRANS_NUM_WEBMAC){
		// copy mac address over:
		while(i<6){server_gwmac[i]=mac[i];i++;}
	}
}

uint16_t Get_DHCP_Config(void);
// DHCP handling. Get the initial IP
uint16_t Get_DHCP_Config(void){
	uint8_t rval= 0;
	uint16_t plen;
	//write mac address in the ethernet controller
	init_mac(mymac);
	
while(rval==0){
	plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
	buf[BUFFER_SIZE]='\0';
	rval=packetloop_dhcp_initial_ip_assignment(buf,plen,mymac[5]);
	}
	
	// we have an IP:
	dhcp_get_my_ip(myip,netmask,gwip);
	client_ifconfig(myip,netmask);
#if 0 //create error notifie through LEDS	
	if (gwip[0]==0){
		// we must have a gateway returned from the dhcp server
		// otherwise this code will not work
	//	PD0LEDON; // error
		while(1); // stop here
	}
#endif	

	// we have a gateway.
	// find the mac address of the gateway (e.g your dsl router).
	get_mac_with_arp(gwip,TRANS_NUM_GWMAC,&arpresolver_result_callback);
		
	return plen;
}

static WORKING_AREA(main_tcp_thread_wa, 512);
static msg_t main_tcp_thread(void *arg){
//int main(void){
        uint16_t dat_p,gPlen;
				uint16_t plen;
        
        /*initialize enc28j60*/
        enc28j60Init(mymac);
        enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
       // _delay_loop_1(0); // 60us
				chThdSleepMicroseconds(60);
        
        // Magjack leds configuration, see enc28j60 datasheet, page 11 
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);
	
			  plen = Get_DHCP_Config();
	
	while(get_mac_with_arp_wait()){
		// to process the ARP reply we must call the packetloop
		plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
		packetloop_arp_icmp_tcp(buf,plen);
	}
	
	init_udp_or_www_server(mymac,myip);
	//www_server_port(MYWWWPORT);
		
	// Main loop of the program
	while(1){
		
	// Gets a packet from the network receive buffer, if one is available.
	// The packet will by headed by an ethernet header.
	//      maxlen  The maximum acceptable length of a retrieved packet.
	//      packet  Pointer where packet data should be stored.
	// Returns: Packet length in bytes if a packet was retrieved, zero otherwise.
	plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
	// http is an ascii protocol. Make sure we have a string terminator.
	buf[BUFFER_SIZE]='\0';
	// DHCP renew IP: for this to work you have to call dhcp_6sec_tick() every 6 sec
	plen=packetloop_dhcp_renewhandler(buf,plen);
			
	// return 0 to just continue in the packet loop and return the position
	// of the tcp data if there is tcp data part
	dat_p=packetloop_arp_icmp_tcp(buf,plen);

	if(dat_p==0){
		//udp_client_check_for_dns_answer(buf,plen);
		continue; // if upd data, we are not interested
	}

			
SENDTCP:
	www_server_reply(buf,gPlen); // send data
	continue;
	} // End of main loop
	
	
#if 0
        //init the ethernet/ip layer:
        init_udp_or_www_server(mymac,myip);
        www_server_port(MYWWWPORT);

        while(1){
                // handle ping and wait for a tcp packet:
                dat_p=packetloop_arp_icmp_tcp(buf,enc28j60PacketReceive(BUFFER_SIZE, buf));

                // dat_p will be unequal to zero if there is a valid http get 
                if(dat_p==0){
                        // do nothing
                        continue;
                }
                // tcp port 80 begin
                if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
                        // head, post and other methods:
                        //
                        // for possible status codes see:
                        // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
                        dat_p=http200ok();
                        dat_p=fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
                        goto SENDTCP;
                }
                if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
                        dat_p=http200ok();
                        dat_p=fill_tcp_data_p(buf,dat_p,PSTR("<p>OK, works great!</p>\n"));
                        goto SENDTCP;
                }
                // all other URLs:
                dat_p=fill_tcp_data_p(buf,0,"HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>");
SENDTCP:
                www_server_reply(buf,dat_p); // send web page data
        }
#endif				
				
        return (0);
}

void start_tcp_threads(void){
 /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(main_tcp_thread_wa, sizeof(main_tcp_thread_wa), NORMALPRIO, main_tcp_thread, NULL);

}
#endif
