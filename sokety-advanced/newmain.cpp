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
#include <linux/ip.h>
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

/* Modifikujte (nebo rozšiřte) předchozí program tak,
 * aby přijímal všechny IP pakety a vypisoval všechny
 * informace ze záhlaví paketu. Fragmentační údaje
 * vypisujte pohromadě a IP adresy odesílatele a
 * příjemce v tečkovém tvaru.h

Pro zpřístupnění povinných položek hlavičky IP paketu lze použít strukturu iphdr:

#include <netinet/ip.h>
struct iphdr;

    * unsigned int version:4: verze IP, tedy 4
    * unsigned int ihl:4: délka záhlaví (včetně volitelných položek), v jednotkách 4 B
    * u_int8_t tos: typ služby, při nepoužití 0
    * u_int16_t tot_len: celková délka IP paketu (hlavička + data) v bytech
    * u_int16_t id: identifikátor paketu přidělený OS
    * u_int16_t frag_off: první tři bity jsou příznaky pro fragmentaci, první vždy 0 (rezervován), druhý DF - zakázání fragmentace, třetí MF - fragment není poslední, zbylých 13 bitů udává posunutí dat fragmentu v původním paketu
    * u_int8_t ttl: doba životnosti paketu (TTL)
    * u_int8_t protocol: protokol vyšší vrstvy, transportní nebo síťový "podprotokol" (např. ICMP)
    * u_int16_t check: kontrolní součet záhlaví
    * u_int32_t saddr: IP adresa odesílatele
    * u_int32_t daddr: IP adresa příjemce

Při odesílání paketu skrze raw socket s pseudoprotokolem IPPROTO_RAW musí být
     * součástí odesílaných dat IP hlavička s vyplněnými (téměř) všemi položkami.
     * Pro výpočet kontrolního součtu vyplněného záhlaví můžeme použít funkci
     * v RFC 1071, pole pro kontrolní součet v záhlaví, ze kterého se součet
     * teprve počítá, je rovno 0. Skutečně odeslaná hlavička nemusí být stejná
     * jako vyplněná, na unixových systémech jsou automaticky vyplňovány kontrolní
     * součet a celková délka, a při nulových hodnotách také identifikátor
     * (při nenulovém hrozí kolize s jiným paketem!) a IP adresa odesílatele.
     * Za IP adresu příjemce se bere ta ze struktury sockaddr_in předaná funkci
     * sendto. Pakety větší než MTU linky nejsou systémem automaticky
     * fragmentovány a jejich velikost je tak omezena na MTU linky.

Pseudoprotokol IPPROTO_RAW ovšem nelze použít pro příjem paketů včetně hlavičky.
     * K tomuto účelu lze použít protokol IPPROTO_IP a volbu socketu IP_HDRINCL
     * (viz dále). Pak je součástí přijímaných dat i IP hlavička. Přijímané IP
     * pakety jsou automaticky systémem defragmentovány, pro příjem jednotlivých
     * fragmentů je nutné použít paketový socket (např. s protokolem ETH_P_IP).
     * Protokol IPPROTO_IP s volbou IP_HDRINCL lze použít i pro odesílání paketů
     * (součástí dat musí být vyplněná IP hlavička).

Volby socketu

Parametry socketu a přenášených protokolů (zejména položky hlaviček)
     * lze upravovat pomocí voleb socketu (socket options).
     * K získání a nastavování hodnot voleb slouží dvojice
     * funkcí getsockopt a setsockopt.

getsockopt(2)
#include <sys/types.h>
#include <sys/socket.h>
int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int getsockopt(SOCKET s, int level, int optname, char* optval, int* optlen);
int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int setsockopt(SOCKET s, int level, int optname, const char* optval, int optlen);

    * getsockopt vrací hodnotu volby optname socketu s na úrovni level na místo
    * na optval a délku hodnoty na místo na optlen (kde je při volání funkce
    * délka místa na optval, socklen_t unsigned long)
    * setsockopt nastavuje hodnotu volby optname socketu s na úrovni level na
    * hodnotu na místě na optval délky optlen
    * úrovně level voleb odpovídají protokolům, kterých se volby týkají, např.
     * IPPROTO_IP, IPPROTO_TCP apod. (je potřeba vložit hlavičkový soubor
     * pro protokol, např. netinet/ip.h nebo netinet/tcp.h), nebo nejvyšší
     * úrovni socketu, SOL_SOCKET
    * volby jsou popsány v socket(7) pro úroveň socketu, ip(7), tcp(7) apod. pro protokolové úrovně
    * vrací 0 nebo (při typu int) hodnotu volby, při chybě -1

Voleb socketu existuje mnoho. Na úrovni protokolů volby ovlivňují každý odesílaný
     * nebo přijímaný paket (IP) nebo segment/datagram (TCP/UDP) a kromě voleb
     * pro parametry protokolu existují i volby odpovídající některým položkám
     * záhlaví protokolu. Např. pro protokol IP, tj. úroveň IPPROTO_IP, jsou
     * volby pro přítomnost IP záhlaví před daty (IP_HDRINCL, pouze u raw socketů,
     * viz výše), u odchozích paketů pro typ služby (IP_TOS), dobu živostnosti paketu
     * (IP_TTL), volitelné položky IP záhlaví (IP_OPTIONS), dále zjištění a nastavení
     * MTU linky (IP_MTU_DISCOVER), aktuální MTU linky (IP_MTU, u socketů spojení),
     * pro skupinové adresování
     * (multicast, IP_ADD_MEMBERSHIP, IP_DROP_MEMBERSHIP, IP_MULTICAST_TTL, IP_MULTICAST_LOOP) a další.

Na úrovni socketu jsou dostupné volby pro obecné parametry socketu pro odesílání,
     * příjem či spojení, např. max. velikosti vstupního (SO_RCVBUF) a výstupního
     * (SO_SNDBUF) bufferu, možnost odesílat/přijímat na/z všesměrové adresy
     * (SO_BROADCAST), priorita odesílaných dat (SO_PRIORITY), časový interval
     * pro odeslání (SO_SNDTIMEO) a příjem (SO_RCVTIMEO) dat před ukončením
     * blokování odesílací nebo přijímací funkce s chybou a další.
    */

    struct ether_header* header;
    header = (struct ether_header*) buffer;

    if (ntohs (header->ether_type) == ETHERTYPE_IP) {

    struct iphdr* data;
    data = (struct iphdr*) (buffer + sizeof(struct ether_header));

    struct in_addr address;

    cout << endl << "version : " << (unsigned int) data->version << endl;
    cout << (unsigned int) ntohs(data->ihl) << ": délka záhlaví (včetně volitelných položek), v jednotkách 4 B" << endl;
    cout << ntohs(data->tos) << ": typ služby, při nepoužití 0" << endl;
    cout << ntohs(data->tot_len) <<" : celková délka IP paketu (hlavička + data) v bytech" << endl;
    cout << ntohs(data->id) << " : identifikátor paketu přidělený OS" << endl;
    cout << ntohs(data->frag_off) << " : první tři bity jsou příznaky pro fragmentaci, první vždy 0 (rezervován), druhý DF - zakázání fragmentace, třetí MF - fragment není poslední, zbylých 13 bitů udává posunutí dat fragmentu v původním paketu" << endl;
    cout << ntohs(data->ttl) << ": doba životnosti paketu (TTL)" << endl;
    cout << ntohs(data->protocol) << " : protokol vyšší vrstvy, transportní nebo síťový podprotokol (např. ICMP)" << endl;
    cout << ntohs(data->check) << " : kontrolní součet záhlaví" << endl;
    address.s_addr = ntohs(data->saddr);
    cout << inet_ntoa(address) << " : IP adresa odesílatele" << endl;
    address.s_addr = ntohs(data->daddr);
    cout << inet_ntoa(address) << " : IP adresa příjemce" << endl;
    }
/*
    if(data->protocol == IPPROTO_TCP) {
        cout << "protokol sitove vrstvy : TCP" << endl;
    } else {
        if(data->protocol == IPPROTO_UDP) {
            cout << "protokol sitove vrstvy : UDP" << endl;
        } else {
            cout << "protokol sitove vrstvy : " << ntohs(data->protocol) << endl << endl;
        }
    }
    */
  }

  /* zrusime socket */
  close (sock);

  return 0;
}
