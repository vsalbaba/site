#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <bits/pthreadtypes.h>
#include <pthread.h>

#define BUFSIZE 1500

using namespace std;
int finish = 0;
int sock, connsock;
struct sockaddr_in address, client_address;
socklen_t client_address_size;
char buffer[BUFSIZE];

void sig_handler(int sig) {
    if (sig == SIGINT)
        finish = 1;
}

void *funkce_vlakna(void *parametry) {
    ssize_t length;
    /* vypisujeme data */
    do {
        /* prijmeme data */
        length = recv(sock, buffer, BUFSIZE, 0);

        if (length == -1) {
            if (errno != EINTR)
                perror("recv error");
            close(sock);
            break;
        }
        printf("away: %s \n", buffer);
    } while (length || !finish);

    /* zrusime socket spojeni = ukoncime spojeni */
    close(connsock);
    return NULL;
}

int main(int argc, char *argv[]) {
    struct sigaction act;
    char ch;
    string input_line;
    ssize_t length, count;

    char *ip_addr;
    int port;
    pthread_t new_thread;
    pthread_attr_t thread_attributes;
    memset(&act, 0, sizeof (act));
    act.sa_handler = sig_handler;
    sigaction(SIGINT, &act, NULL);

    /* vytvorime socket */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket error");
        return -1;
    }

    if (argc == 3) {
        ip_addr = strdup(argv[1]);
        port = atoi(argv[2]);
    } else if (argc == 2) {
        port = atoi(argv[1]);
    }


    address.sin_family = PF_INET;
    address.sin_port = htons(port);

    /*
     * klient
     */
    if (argc == 3) {
        /*
         * klient
         */

        inet_aton(ip_addr, &(address.sin_addr));

        /* pripojime socket na adresu = navazeme spojeni */
        if (connect(sock, (struct sockaddr *) & address, sizeof (address)) == -1) {
            perror("connect error");
            close(sock);
            return -1;
        }
    } else if (argc == 2) {
        /* 
         * server
         */

        /* nastavime lokalni adresu, na ktere budeme naslouchat
           (odkudkoliv na portu port) */

        address.sin_addr.s_addr = INADDR_ANY;

        /* pripojime socket na adresu = pojmenujeme socket */
        if (bind(sock, (struct sockaddr *) & address, sizeof (address)) == -1) {
            perror("bind error");
            close(sock);
            return -1;
        }

        /* a naslouchame na socketu, s frontou pozadavku na spojeni delky 10 */
        if (listen(sock, 10) == -1) {
            perror("listen error");
            close(sock);
            return -1;
        }

        client_address_size = sizeof (client_address);

        sock = accept(sock, (struct sockaddr *) & client_address,
                &client_address_size);

        if (connsock == -1) {
            if (errno != EINTR) {
                perror("accept error");
                close(sock);
                return -1;
            }
        }

    }


    /*
     *vytvorim si vlakno v kterem bude server nacitat data
     */

    pthread_attr_init(&thread_attributes);
    pthread_create(&new_thread, &thread_attributes, &funkce_vlakna, NULL);

    /* nacitame data */
    while (!finish) {
        input_line.clear();
        while ((ch = getchar()) && (ch != '\n')) {
            input_line += ch;
        }

        /* odesleme data */

        for (int i = 0; i < (input_line.size() / BUFSIZE) + 1; i++) {
            length = send(sock, (void *) input_line.substr(BUFSIZE*i, BUFSIZE * (i + 1)).c_str(), BUFSIZE, 0);
            if (length == -1) {
                perror("send error");
                close(sock);
                return -1;
            }
        }
    }

    /* zrusime socket */
    close(sock);

    return 0;
}