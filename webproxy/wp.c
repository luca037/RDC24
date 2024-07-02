#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <signal.h>

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
    t = listen(s, 10);
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
        //close(s2);
        s2 = accept(s, (struct sockaddr*) &remote, &len);
        
        /* Faccio gestire ai processi figli le richieste */
        if (fork()) continue;

        if (s2 == -1) {
            perror("Accept fails");
            exit(1);
        }

        // Stampo informazioni client.
        printf("\nClient port = %u, addr = %X\n\n", htons(remote.sin_port), htons(remote.sin_addr.s_addr));

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
        char *method, *url, *http_version;
        method = h[0].value;
        for (i = 0; h[0].value[i] != ' '; i++) ;
        h[0].value[i] = 0;
        i += 1;

        url = h[0].value + i;
        for ( ; h[0].value[i] != ' '; i++) ;
        h[0].value[i] = 0;
        i += 1;

        http_version = h[0].value + i; // Il terminatore è stato inserito
                                       // (al posto di '\r') nel parser
                                       // dell'header.

// ### Response ###
        if (!strcmp(method, "GET")) {
            char* scheme = url;
            printf("url = %s\n", url);

            // Parsing dell'scheme, hostame e filename.
            // Riferimento: GET http://www.example.com/file/file, dove
            //      scheme   = http
            //      hostname = www.example.come
            //      filename = /file/file
            for (i = 0; url[i] != ':' && url[i]; i++) ;
            if (url[i] == ':') {
                url[i++] = 0; // Terminatore di stringa.
            } else {
                printf("Parse errore, expected ':'\n");
                exit(1);
            }
            if (url[i] != '/' || url[i+1] != '/') {
                printf("Parse errore, expected '//'\n");
                exit(1);
            }
            i = i + 2; // Salto i '//'
            char* hostname = url + i;
            for ( ; url[i] != '/' && url[i]; i++) ;
            if (url[i] == '/') {
                url[i++] = 0; // Terminatore di stringa.
            } else {
                printf("Parse errore, expected '/'\n");
                exit(1);
            }
            char* filename = url + i;
            printf("Schema: %s, hostname: %s, filename: %s\n", scheme, hostname, filename);
            // Fine parsing.

            // Risolvo il nome.
            struct hostent* he = gethostbyname(hostname);
            printf("ip server: %d.%d.%d.%d\n", (unsigned char) he->h_addr[0], (unsigned char) he->h_addr[1], (unsigned char) he->h_addr[2],  (unsigned char) he->h_addr[3]);

            // Creazione client del proxy.
            int s3 = socket(AF_INET, SOCK_STREAM, 0);
            if (s3 == -1) {
                perror("Scoket fails");
                exit(1);
            }

            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons(80);
            server.sin_addr.s_addr = *(unsigned int*) (he->h_addr);

            t = connect(s3, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
            if (t == -1) {
                perror("Connect fails");
                exit(1);
            }

            // Inoltro la richiesta al server.
            sprintf(request, "GET /%s HTTP/1.1\r\nHost:%s\r\nConnection:close\r\n\r\n", filename, hostname);
            write(s3, request, strlen(request));
            
            // Inoltro la risposta del server al client.
            char buffer[2001];
            while (t = read(s3, buffer, 2000)) {
                write(s2, buffer, t);
            }
            close(s3);
            exit(0);
        } else if (!strcmp(method, "CONNECT")) {
            char* hostname = url;
            // Riferimento: connect hostname:port
            for (i = 0; url[i] != ':' && url[i]; i++) ;
            url[i] = 0;
            char* port = url + i + 1;
            printf("hostname: %s, port: %s\n", hostname, port);

            // Risolvo il nome.
            struct hostent* he = gethostbyname(hostname);
            printf("ip server: %d.%d.%d.%d\n", (unsigned char) he->h_addr[0], (unsigned char) he->h_addr[1], (unsigned char) he->h_addr[2],  (unsigned char) he->h_addr[3]);

            // Creazione client del proxy.
            int s3 = socket(AF_INET, SOCK_STREAM, 0);
            if (s3 == -1) {
                perror("Scoket fails");
                exit(1);
            }

            struct sockaddr_in server;
            server.sin_family = AF_INET;
            server.sin_port = htons((unsigned short) atoi(port));
            server.sin_addr.s_addr = *(unsigned int*) (he->h_addr);

            t = connect(s3, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
            if (t == -1) {
                perror("Connect fails");
                exit(1);
            }
            sprintf(response, "HTTP/1.1 200 Established\r\n\r\n");

            char response2[2001];
            char request2[2001];
            int pid = fork();
            if (!pid) { // Child.
                while (t = read(s2, request2, 2000)) {
                    write(s3, request2, t);
                }
                exit(0);
            } else { // Parent.
                while (t = read(s3, response2, 2000)) {
                    write(s2, response2, t);
                }
                kill(pid, SIGTERM);
                close(s3);
            }
        } else {
            sprintf(response, "HTTP/1.1 501 Not Implemented\r\n\r\n");
            write(s2, response, strlen(response));
        }
        close(s2);
        exit(0);
    }
    close(s);

    return 0;
}
