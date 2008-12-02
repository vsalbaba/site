/*
 * Př. Implementujte zjednodušenou verzi programu ping. Program odešle několik
 * ICMP echo žádostí na IP adresu zadanou jako parametr programu a po odeslání
 * každé přijme odpovídající echo odpověď. Nastavte TTL odchozích IP paketů
 * na 255 a časový interval pro příjem odpovídající odpovědi na 5 sekund
 * (pomocí volby socketu SO_RCVTIMEO). U každé dvojice žádost-odpověď
 * vypište čas mezi nimi (round trip time, RTT) pomocí funkce gettimeofday(2).
 * Pro výpočet kontrolního součtu ICMP záhlaví použijte funkci z RFC 1071.
 */
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include <netdb.h>

#define BUFSIZE 1500

int finish = 0;

/* Funkce checksum je opsaná z příslušných RFC dokumentů. */
uint16_t checksum(uint16_t *addr, unsigned count)
{

  uint32_t sum = 0;
  while (count > 1)  {
    sum += *(addr++);
    count -= 2;
  }

  // mop up an odd byte, if necessary
  if (count > 0) {
    sum += *(unsigned char *)addr;
  }

  // add back carry outs from top 16 bits to low 16 bits
   while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

  return ~sum; // truncate to 16 bits
}



void sig_handler(int sig) {
    if (sig == SIGINT)
        finish = 1;
}

int main(int argc, char *argv[]) {
    hostent *host;
    host = gethostbyname(argv[1]);
    struct sigaction act;
    struct icmphdr my_icmp_header;
    int sock;
    unsigned int ttl = 255;
    timeval my_tv;
    sockaddr_in my_sockaddr_in;
    memset(&act, 0, sizeof (act));
    act.sa_handler = sig_handler;

    sigaction(SIGINT, &act, NULL);

    /* vytvorime raw socket */
    sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sock == -1) {
        perror("socket error");
        return -1;
    }

    // Nastavte TTL odchozích IP paketů na 255
    setsockopt(sock, IPPROTO_IP, IP_TTL,
            (const char *) & ttl, sizeof (ttl));

    //Nastavte časový interval pro příjem odpovídající odpovědi na 5 sekund (pomocí volby socketu SO_RCVTIMEO)
    my_tv.tv_sec = 5;
    my_tv.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*) & my_tv, sizeof (my_tv));

    //vytvorim icmp paket
    my_icmp_header.type = ICMP_ECHO;
    my_icmp_header.code = 0;
    my_icmp_header.un.echo.id = getpid();

    //naplneni sockaddr_in
    my_sockaddr_in.sin_family = AF_INET;
    my_sockaddr_in.sin_port = 0;
    memcpy(&my_sockaddr_in.sin_addr, host->h_addr, host->h_length);


    my_icmp_header.checksum = 0;
    my_icmp_header.un.echo.sequence = 0;

    my_icmp_header.checksum = checksum((unsigned short *)&my_icmp_header, sizeof(my_icmp_header));

    printf("odeslali sme data\n");
    sendto(sock, (char *)&my_icmp_header, sizeof(icmphdr), 0, (sockaddr *)&my_sockaddr_in, sizeof(sockaddr));
    /* zrusime socket */
    close(sock);

    return 0;
}