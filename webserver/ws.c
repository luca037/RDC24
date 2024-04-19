#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

/* Struttura di un header:
        Nome | Valore
*/
struct headers {
    char* name;
    char* value;
} h[100];

int main(void) {
    char request[3001];
    char response[3001];

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        perror("Socket fails");
        return 1;
    }

    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(9010);
    srvaddr.sin_addr.s_addr = 0; // 0 => indirzzo della macchina.


    /* Evita errori generati dalla bind su indirizzi utilizzati
       recentemente */
    int yes = 1;
    int t = setsockopt(s, SOL_SOCKET,SO_REUSEADDR, &yes, sizeof(int));
    if (t == -1) {
        perror("Setsockopt fails");
        return 1;
    }

// ### Bind ###
    /* Rendo passovo il socket sfruttando la bind */
    t = bind(s, (struct sockaddr*) &srvaddr, sizeof(struct sockaddr_in));
    if (t == -1) {
        perror("Bind fails");
        return 1;
    }

// ### Listen ###
    /* Rendo il socket passivo e fisso la dimensione della coda delle richieste
       pendenti a 5. */
    t = listen(s, 5);
    if (t == -1) {
        perror("Listen fails");
        return 1;
    }

// ### Accept ###
    /* Accetto le richieste. */
    struct sockaddr_in remote;
    int len = sizeof(struct sockaddr);

    int s2; // File descriptor del socket tornato da accept.
    while (1) {
        close(s2);
        s2 = accept(s, (struct sockaddr*) &remote, &len);
        
        /* Faccio gestire ai processi figli le richieste */
        if (fork()) continue;

        if (s2 == -1) {
            perror("Accept fails");
            exit(1);
        }

        printf("\nClient port = %u, addr = %u\n", remote.sin_port, remote.sin_addr.s_addr);

// ### Parsing Header ###
        char hbuf[10000]; // Buffer header.

        h[0].name = "Request Line";
        h[0].value = hbuf;

        int i, j;
        for (i = 0, j = 0; read(s2, hbuf + i, 1); i++) {

            // Setto il puntatore al Valore.
            if (hbuf[i] == ':' && h[j].value == NULL) {
                h[j].value = &hbuf[i+1];
                hbuf[i] = 0; // Terminatore di stringa.
            }

            // Setto il puntatore al Nome o termino.
            else if (i > 0 && hbuf[i] == '\n' && hbuf[i-1] == '\r') {
                hbuf[i-1] = 0; // Terminatore di stringa.

                /* Controllo se l'header è terminato.
                   L'header termina quando ho un doppio 'CRLF'. */
                if (h[j].name[0] == 0) {
                    break;
                }
                // Setto il puntatore al Nome.
                h[++j].name = &hbuf[i+1];
            }
        }

        // Stampa header.
        for (i = 0; i < j; i++) {
            printf("%s:%s\n", h[i].name, h[i].value);
        }
        printf("\n");

// ### Parsing Request-Line ###
        // Request-Line = Method SP Request-URI SP HTTP-Version CRLF
        char *method, *filename, *http_version;
        method = h[0].value;
        for (i = 0; h[0].value[i] != ' '; i++) ;
        h[0].value[i] = 0;
        i += 1;

        filename = h[0].value + i;
        for ( ; h[0].value[i] != ' '; i++) ;
        h[0].value[i] = 0;
        i += 1;

        http_version = h[0].value + i; // Il terminatore è stato inserito
                                       // (al posto di '\r') nel parser
                                       // dell'header.

// ### Response ###
        /* Full-Response */
        FILE* fin = fopen(filename + 1, "rt"); // Il '+1' per rimuovere il path
                                               // relativo.
        if (fin == NULL) {
            sprintf(response, "HTTP/1.1 404 NOT FOUND\r\nConnection:close\r\n\r\n<html><h1>404 File %s found</h1></html>", filename);
            write(s2, response, strlen(response));
            close(s2);
            exit(1);
        }

        sprintf(response, "HTTP/1.1 200 OK\r\nConnection:close\r\n\r\n");
        write(s2, response, strlen(response));

        char entity[1000];
        while (!feof(fin)) {
            fread(entity, 1, 1000, fin);
            write(s2, entity, 1000);
        }

        //exit(0); // Per far terminare il processo figlio.
    }

    return 0;
}
