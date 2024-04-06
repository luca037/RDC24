#include <stdio.h>
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
    server.sin_port = htons(80); 

    unsigned char* p = (unsigned char*) &server.sin_addr.s_addr;
    /* Scrivento l'indirizzo in questo modo è già in notazione big endian. */
    p[0] = 216; p[1] = 58; p[2] = 209; p[3] = 36;

    int tmp;
    tmp = connect(s, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
    if (tmp == -1) {
        perror("Connect fails");
        return 1;
    }

// ### Write ###
    /* Full-Request */
    write(s, "GET / HTTP/1.0\r\n\r\n", 18);

// ### HTTP parser ###
    /*
       Vogliamo realizzare un parser che ci permetta di ricavare
       solamente l'Entity-Body della Full-Response e che di conseguenza
       scerti l'header.

       Per realizzare un parser efficiente bisogna che questo lavori
       mentre i dati stanno arrivando, non possiamo aspettare che
       arrivino tutti prima di iniziare.
     */

    const int dim = 100000;
    char response[dim];

    // Scarto l'header.
    h[0].name = "Status line\0";
    h[0].value = response;
    int i, j;
    for (i = 0, j = 0; read(s, response + i, 1); i++) {

        // Setto il puntatore al Valore.
        if (response[i] == ':' && h[j].value == NULL) {
            h[j].value = &response[i+1];
            response[i] = 0; // Terminatore di stringa.
        }

        // Setto il puntatore al Nome.
        else if (i != 0 && response[i] == '\n' && response[i-1] == '\r') {
            // Termine header.
            if (i > 2 && response[i-2] == '\n' && (response[i-3] == 0 || response[i-3] == '\r')) {
                break;
            }
            h[++j].name = &response[i+1];
            response[i-1] = 0; // Terminatore di stringa.
        }
    }
    response[i] = 0; // Terminatore di stringa.
    printf("Dimensione header: %d byte\n", i);
    //printf("\nj = %d\n", j);

    // Stampa header.
    printf("\nContenuto header:\n");
    for (i = 0; i < j; i++) {
        printf("%s:%s\n", h[i].name, h[i].value);
    }

    return 0;
}
