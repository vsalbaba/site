/* 
 * File:   newmain.cpp
 * Author: vojta
 *
 * Created on 13. listopad 2008, 12:39
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <iostream>

using namespace std;
/*
 * Napište program pro příjem všech linkových rámců vypisující následující informace: 
 * délka rámce
 * linkový protokol
 * adresy odesílatele
 * adresy příjemce (v dvojtečkovém tvaru)
 * protokol síťové vrstvy 
 */

#define BUFFLEN 1500

int finish = 0;

void sig_handler (int sig)
{
  if (sig == SIGINT)
    finish = 1;
}

int main (int argc, char *argv[])
{
  struct sigaction act;
  
  int sock, i;
  ssize_t length;
  char buffer[BUFFLEN];
  struct sockaddr_ll address;
  socklen_t address_size;

  memset (&act, 0, sizeof(act));
  act.sa_handler = sig_handler;
  sigaction (SIGINT, &act, NULL);
  
  /* vytvorime socket */
  sock = socket (PF_PACKET, SOCK_RAW, htons (ETH_P_ALL));

  if (sock == -1) {
    perror ("socket error");
    return -1;
  }

  address_size = sizeof(address);
  
  while (!finish) {
    /* prijmeme data */
    length = recvfrom (sock, buffer, BUFFLEN, 0, (struct sockaddr *)&address,
		       &address_size);
    
    if (length == -1) {
      if (errno != EINTR) {
	perror ("recvfrom error");
	close (sock);
	return -1;
      }
      else
	continue;
    }
    
    /* vypiseme data */
//    for (i = 0; i < length; i++)
//      printf ("%02hhx ", buffer[i]);
//    printf ("(%i bytes)\n", length);
//  
 /*
 * Napište program pro příjem všech linkových rámců vypisující následující informace: 
 * délka rámce
 * linkový protokol
 * adresy odesílatele
 * adresy příjemce (v dvojtečkovém tvaru)
 * protokol síťové vrstvy 
 */
    
    struct ether_header* header;
    header = (struct ether_header*) buffer; 
    /* 
    * kdo by to byl cekal, pretypovavame buffer abychom dostali ether_header)
    */
    
    cout << "delka ramce : " << length << endl;
    cout << "linkovy protokol : " << address.sll_protocol << endl;
    cout << "adresa odesilatele : " <<  ether_ntoa((ether_addr*)header->ether_shost) << endl;
    cout << "adresa prijemce : " <<  ether_ntoa((ether_addr*)header->ether_dhost) << endl << endl;
    /*
    * tusite ja ziskat protokol sitove vrstvy? napiste to do komentare!
    */
  }
    
  /* zrusime socket */
  close (sock);
  
  return 0;
}
