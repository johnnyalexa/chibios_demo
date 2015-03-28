/*
 * m88_eth_01.c
 *
 * Created: 3/31/2014 8:31:28 PM
 *  Author: John
 */ 


/*********************************************
 * vim:sw=8:ts=8:si:et
 * To use the above modeline in vim you must have "set modeline" in your .vimrc
 * Author: Guido Socher
 * Copyright: GPL V2
 *
 * Read the silicon revision of the ENC28J60 via http or UDP
 *
 * http://tuxgraphics.org/electronics/
 * Chip type           : Atmega88/168/328/644 with ENC28J60
 *********************************************/
 #if 1
#include "hal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ip_arp_udp_tcp.h"
#include "enc28j60.h"
#include "dhcp_client.h"
#include "net.h"
#include "timeout.h"

#define PSTR (const char *)


// please modify the following two lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? x S L F x x 
static uint8_t defaultmac[6] = {0x00,0x53,0x4C,0x46,0x00,0x03};
static uint8_t defaultip[4] = {192,168,0,7}; // aka http://10.0.0.29/

// listen port for www
#define MYWWWPORT 8082
//// listen port for udp
#define MYUDPPORT_DEFAULT 4601

#define BUFFER_SIZE 550
static uint8_t buf[BUFFER_SIZE+1];

uint16_t http200ok(void);
uint16_t http200ok(void)
{
        return(fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

uint16_t print_webpage(uint8_t *buf);
// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
        char vstr[5];
        uint16_t plen;
        plen=http200ok();
        plen=fill_tcp_data_p(buf,plen,PSTR("<center><p>ENC28J60 silicon rev is: B"));
        // convert number to string:
      //  itoa((enc28j60getrev()),vstr,10);
	       sprintf(vstr,"%d",(int)enc28j60getrev());
	
        plen=fill_tcp_data(buf,plen,vstr);
        plen=fill_tcp_data_p(buf,plen,PSTR("</center><hr><br>SLF.RO\n"));
        return(plen);
}

static WORKING_AREA(main_tcp_thread_wa, 512);
static msg_t main_tcp_thread(void *arg){
	
	
        uint16_t dat_p,plen;
        uint8_t payloadlen=0;
        char str[20];
        uint8_t SetNewConfig = 0;
		int config_rc = 0;

		int scan_tmp[5];
		int scanf_rc =0 ;
		int reset_rc=0;
		int led_count=0;
				chThdSleepMicroseconds(60);
        //_delay_loop_1(0); // 60us
        
        /*initialize enc28j60*/
        enc28j60Init(defaultmac);
		//	LOG("Init enc");
        enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
		//	LOG("change enc clk to 12.5mhz");
			chThdSleepMicroseconds(60);
        //_delay_loop_1(0); // 60us
        
        /* Magjack leds configuration, see enc28j60 datasheet, page 11 */
        // LEDB=yellow LEDA=green
        //
        // 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
        // enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
        enc28j60PhyWrite(PHLCON,0x476);

        
        //init the ethernet/ip layer:
        init_udp_or_www_server(defaultmac,defaultip);	
		//	LOG("Init udp server and start tcp");
#ifdef WWW_server			
        www_server_port(MYWWWPORT);
#endif		
//		_delay_ms(2000);
	//	USART_print("@     ");
	//	_delay_ms(20);
chThdSleepMicroseconds(1000);
		
		//wdt_enable(WDTO_8S); //enable watchdog to avoid hangs
        while(1){
					//wdt_reset();	//feed the dog

                // handle ping and wait for a tcp packet:
                plen=enc28j60PacketReceive(BUFFER_SIZE, buf);
                dat_p=packetloop_arp_icmp_tcp(buf,plen);

                /* dat_p will ne unequal to zero if there is a valid 
                 * http get */
                if(dat_p==0){
                        // check for udp
                        goto UDP;
                }
#ifdef WWW_server
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
                        dat_p=print_webpage(buf);
                        goto SENDTCP;
                }else{
                        dat_p=fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
                        goto SENDTCP;
                }
SENDTCP:
                www_server_reply(buf,dat_p); // send web page data
                continue;
                // tcp port 80 end
#endif //WWW_server				
                //--------------------------
                // udp start, we listen on udp port 1200=0x4B0
UDP:
                // check if ip packets are for us:
                if(eth_type_is_ip_and_my_ip(buf,plen)==0){
                        continue;
                }
                if (buf[IP_PROTO_P] == IP_PROTO_UDP_V &&\
				    buf[UDP_DST_PORT_H_P] == (MYUDPPORT_DEFAULT>>8) &&\
					buf[UDP_DST_PORT_L_P] == (MYUDPPORT_DEFAULT&0xff)) {
                        payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
                        // you must sent a string starting with v
                        // e.g udpcom version 10.0.0.24
  						if (buf[UDP_DATA_P]=='*' ){
							  scanf_rc = sscanf(&buf[UDP_DATA_P],"*%d.%d.%d.%d:%d", 
												&scan_tmp[0],
												&scan_tmp[1],
												&scan_tmp[2],
												&scan_tmp[3],
												&scan_tmp[4]
												);
																	
											//	USART_Transmit(0x30+scanf_rc);
								if(scanf_rc !=5 )				
									strcpy(str,"Config error: Please use format *IP:port");
								else{

									strcpy(str,"Config set OK. Restarting...");

								}
								
						}else
						if (buf[UDP_DATA_P]==':' ){
							scanf_rc = sscanf(&buf[UDP_DATA_P],":%x:%x",
							&scan_tmp[0],
							&scan_tmp[1]						
							);
							
							//	USART_Transmit(0x30+scanf_rc);
							if(scanf_rc !=2 )
							strcpy(str,"Config error: Please use format :a4:92 as mac5 and mac6!");
							else{

								strcpy(str,"Config set OK. Restarting...");
	
							}
							
						}else
						
						{

                               strcpy(str,"OK");

							   
                        }
                        make_udp_reply_from_request(buf,str,strlen(str),MYUDPPORT_DEFAULT);
                }
        }
		/// Reach here only if new config must be set
		
CONFIG_SET:		
		//USART_print("Rst_AVR");
		//Reset_AVR();
        return (0);
}

void start_tcp_threads(void){
 /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(main_tcp_thread_wa, sizeof(main_tcp_thread_wa), NORMALPRIO, main_tcp_thread, NULL);

}
#endif
