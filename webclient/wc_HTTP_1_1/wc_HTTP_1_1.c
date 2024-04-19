#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

struct chunk {
    char* size;
    char* data;
} c[100];

int main() {
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(80); 

    unsigned char* p = (unsigned char*) &server.sin_addr.s_addr;
    p[0] = 216; p[1] = 58; p[2] = 209; p[3] = 36;

    int tmp;
    tmp = connect(s, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
    if (tmp == -1) {
        perror("Connect fails");
        return 1;
    }

    // Request in HTTP 1.1.
    //char* request = "GET /ppp HTTP/1.1\r\n\r\n"; // Content-Length: x byte
    char* request = "GET / HTTP/1.1\r\n\r\n"; // Transfer-Encoding: chunked
    write(s, request, strlen(request));

// ### HTTP parser ###
    // Scarto l'Header.
    char hbuf[10000]; // Buffer header.

    h[0].name = "Status line";
    h[0].value = hbuf;

    int i, j;
    for (i = 0, j = 0; read(s, hbuf + i, 1); i++) {

        // Setto il puntatore al Valore.
        if (hbuf[i] == ':' && h[j].value == NULL) {
            h[j].value = &hbuf[i+1];
            hbuf[i] = 0; // Terminatore di stringa.
        }

        // Setto il puntatore al Nome o termino.
        else if (i != 0 && hbuf[i] == '\n' && hbuf[i-1] == '\r') {
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
    printf("Dimensione header: %d byte\n", i);

    // Stampa header e cotrollo se è presente Content-Length.
    int len = 0;
    printf("\nContenuto Header:\n");
    for (i = 0; i < j; i++) {
        printf("%s:%s\n", h[i].name, h[i].value);

        // Prelevo la lunghezza entity body.
        if (!strcmp(h[i].name, "Content-Length")) {
            len = atoi(h[i].value);
        }
    }
    

    // Se è stato specificato Content-Lenght.
    if (len) {
        char response[100000];

        // Salvo l'Entity Body.
        for (i = 0; tmp = read(s, response + i, len - i); i += tmp) ;
        response[i] = 0; // Terminatore di stringa.

        // Stampa contenuto Entity Body.
        printf("\nContenuto Entity Body:\n%s\n", response);
    } 
    // Se viene utilizzato il chunk transfer.
    else {
        char chunked_body[100000];

        c[0].size = chunked_body;
        for (i = 0, j = 0; tmp = read(s, chunked_body + i, 1); i++) {
            if (chunked_body[i] == '\n' && chunked_body[i-1] == '\r') {
                chunked_body[i-1] = 0;

                if (c[j].size == &chunked_body[i+1]) continue;

                c[j].data = &chunked_body[i+1];

                int s = (int) strtol(c[j].size, NULL, 16);
                printf("\nsize = %d\n", s);
                printf("sizeHex = %s\n", c[j].size);
                if (!s) break;

                c[++j].size = &chunked_body[i+1 + s+2];
            }
        }

        // Stampa contenuto Entity Body.
        int dim_page = 0;
        for (i = 0; i < j; i++) {
            printf("%s", c[i].data);
            dim_page += (int) strtol(c[i].size, NULL, 16);
        }
        printf("\ndim_page = %d\n", dim_page);

        // Salvo il trailer.
        char tbuf[100];
        struct headers t[100];

        int i, j;
        for (i = 0, j = 0; read(s, tbuf + i, 1); i++) {

            // Setto il puntatore al Valore.
            if (tbuf[i] == ':' && t[j].value == NULL) {
                t[j].value = &tbuf[i+1];
                tbuf[i] = 0; // Terminatore di stringa.
            }

            // Setto il puntatore al Nome o termino.
            else if (i > 0 && tbuf[i] == '\n' && tbuf[i-1] == '\r') {
                tbuf[i-1] = 0; // Terminatore di stringa.

                /* Se è presente un trailr, allora devo controllare la
                   presenza di un doppio CRLF. Se non è presente nessun
                   leggerò solamente un CRLF. Quindi se nei primi due byte
                   letti trovo '\r\n', allora posso terminare. */
                if ((t[j].name != NULL && t[j].name[0] == 0) || i == 1) {
                    break;
                }
                // Setto il puntatore al Nome.
                t[++j].name = &tbuf[i+1];
            }
        }

        // Stampa trailer.
        int len = 0;
        printf("\nContenuto Trailer:\n");
        for (i = 0; i < j; i++) {
            printf("%s:%s\n", t[i].name, t[i].value);
        }
    }
    
    return 0;
}
