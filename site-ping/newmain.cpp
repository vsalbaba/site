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
long checksum(unsigned char *addr, int count) {
    /* Compute Internet Checksum for "count" bytes
     *         beginning at location "addr".
     */
    register long sum = 0;

    while (count > 1) {
        /*  This is the inner loop */
        sum += *(unsigned short *) addr++;
        count -= 2;
    }

    /*  Add left-over byte, if any */
    if (count > 0)
        sum += *(unsigned char *) addr;

    /*  Fold 32-bit sum to 16 bits */
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

void sig_handler(int sig) {
    if (sig == SIGINT)
        finish = 1;
}

int main(int argc, char *argv[]) {
    hostent *host;
    host = gethostbyname(argv[1]);
    int p = 0;
    struct sigaction act;
    struct icmphdr my_icmp_header;
    int sock, i;
    ssize_t length;
    char buffer[BUFSIZE];
    struct iphdr *ip;
    struct icmphdr *icmp;
    struct sockaddr_in address;
    socklen_t address_size;
    struct timeval tv;
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

    //vytvorime icmp paket
    my_icmp_header.type = ICMP_ECHO;
    my_icmp_header.code = 0;
    my_icmp_header.un.echo.id = getpid();

    //naplneni sockaddr_in
    my_sockaddr_in.sin_family = AF_INET;
    my_sockaddr_in.sin_port = 0;
    memcpy(&my_sockaddr_in.sin_addr, host->h_addr, host->h_length);

    printf("odeslali sme data\n");
    my_icmp_header.checksum = 0;
    my_icmp_header.un.echo.sequence = p;
    my_icmp_header.checksum = checksum((unsigned char *)&my_icmp_header, sizeof(my_icmp_header));

    sendto(sock, (char *)&my_icmp_header, sizeof(icmphdr), 0, (sockaddr *)&my_sockaddr_in, sizeof(sockaddr));


    address_size = sizeof (address);

    while (!finish) {
        /* prijmeme data */
        length = recvfrom(sock, buffer, BUFSIZE, 0, (struct sockaddr *)&address,
                &address_size);

        if (length == -1) {
            if (errno != EINTR) {
                perror("recvfrom error");
                close(sock);
                return -1;
            } else
                continue;
        }

        /* vypiseme cas */
        gettimeofday(&tv, NULL);
        printf("%li.%03li ms\n", tv.tv_sec, tv.tv_usec);

        /* vypiseme data */
        ip = (struct iphdr *) buffer;
        icmp = (struct icmphdr *) & buffer[ip->ihl * 4];
        printf("%s: ", inet_ntoa(address.sin_addr));
    }

    //odesleme data

    /* zrusime socket */
    close(sock);

    return 0;
}