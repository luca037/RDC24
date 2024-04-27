#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

/* Struttura di un header:
        Nome | Valore
*/
struct headers {
    char* name;
    char* value;
} h[100];

int main() {
// ### Creazione socket ###
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);

// ### Connect ###
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(9010);

    unsigned char* p = (unsigned char*) &server.sin_addr.s_addr;
    /* Scrivento l'indirizzo in questo modo è già in notazione big endian. */
    p[0] = 88; p[1] = 80; p[2] = 187; p[3] = 84; // Server unipd.
    //p[0] = 216; p[1] = 58; p[2] = 209; p[3] = 36; // Server Google.
    //p[0] = 127; p[1] = 0; p[2] = 0; p[3] = 1; // Server locale.

    int tmp;
    tmp = connect(s, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
    if (tmp == -1) {
        perror("Connect fails");
        return 1;
    }

// ### Write ###
    // Full-Request
    char* request = "GET /exec/ls HTTP/1.0\r\n\r\n";
    write(s, request, strlen(request));

// ### HTTP parser ###
    // Scarto l'Header.
    char hbuf[10000]; // Buffer header.

    h[0].name = "Status line\0";
    h[0].value = hbuf;

    int i, j;
    for (i = 0, j = 0; read(s, hbuf + i, 1); i++) {

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
    printf("Dimensione header: %d byte\n", i);

    // Stampa header.
    printf("\nContenuto Header:\n");
    for (i = 0; i < j; i++) {
        printf("%s:%s\n", h[i].name, h[i].value);
    }

    // Prelevo l'Entity Body.
    const int dim = 100000;
    char response[dim];
    for (i = 0; tmp = read(s, response + i, (dim - 1) - i); i += tmp) ;
    response[i] = 0; // Terminatore di stringa.
    printf("\nDimensione Entity Body: %d byte\n", i);

    // Stampa Entity Body.
    printf("\nContenuto Entity Body:\n%s\n", response);

    return 0;
}
