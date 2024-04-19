#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
// ### Creazione socket ###
    int s;
    s = socket(AF_INET, SOCK_STREAM, 0);

     /* La funzione socket ritorna un file descriptor. Stampando il valore di 's'
        ci viene restitito il valore 3. Si tratta di un indice di una tabella
        memorizzata in kernel space in cui si trovano i vari stream aperti.
        In particolare:
             - Il valore 0 è associato allo standard output.
             - Il valore 1 è associato allo standard input.
             - IL valore 2 è associato allo standard error.
        Se aprissimo un file prima di lanciare la chiamata a 'socket', allora
        il file avrebbe 3 come valore associato e 's' avrebbe 4. */
    
    //printf("s = %d\n", s);

    /* Se s = -1 allora signigica che l'end point non è stato creato perché c'è
       stato un error. In tal caso la variabile globale 'errno' (error number)
       viene setta ad uno specifico valore. */

    //s = socket(14141, SOCK_STREAM, 0); // Per generare un errore.
    //if (s == -1) {
    //    printf("errno = %d\n", errno); // errno = EAFNOSUPPORT = 97.
    //    perror("Socket fallita");
    //    return 1;
    //}

// ### Connect ###
    struct sockaddr_in server;
    server.sin_family = AF_INET;

    /* Da little endian (mio pc) a big endian (network order).
       Il network order è il big endian (scelta arbitraria). 
       In questo modo tutti i tipi di periferiche (big o little endian)
       possono comunicare. */
    server.sin_port = htons(80); 

    /* L'indirizzo IPv4 è composto da 4 byte.
       Per poter scrivere l'indirizzo come campo di 'server', possiamo
       sfruttare un puntatore a 'char' e scrivere i byte uno per volta.
       Un'altro modo è quello di trasformare l'indirizzo in binario
       poi di nuovo in decimale e poi usare 'htonl' per passare a big
       endian. */
    unsigned char* p = (unsigned char*) &server.sin_addr.s_addr;

    /* Scrivento l'indirizzo in questo modo è già in notazione big endian. */
    p[0] = 216; p[1] = 58; p[2] = 209; p[3] = 36;

    /* Metodo alternativo: è meno leggibile e richiede più passaggi. */
    //server.sin_addr.s_addr = htonl(3627733284);

    /* * Far fallire la connect *
       Per fare fallire la connect basta inserire un indirizzo casuale.
       Si nota che il programma rimane bloccato in attesa: sta aspettando
       che arrivi la Confirm da parte del server. Dato che l'indirizzo
       specificato non esiste, il programma rimane in attesa fino al tempo
       massimo, dopodiché 'connect' termina con l'errore 'Connection 
       timed out'. */
    //p[0] = 16; p[1] = 58; p[2] = 209; p[3] = 36;

    /* Un altro motivo per il quale la connect può fallire è se la connessione
       con il server viene rifiutata (errore 'Connection refused'). */
    //p[0] = 127; p[1] = 0; p[2] = 0; p[3] = 1; // Mi collego a me stesso.
    //server.sin_port = htons(77); // Non c'è un socket collegato a questa porta.

    /* Posso verificare se la connesione è andata a buon fine con il
       comando 'netstat -nt | grep "<indirizzo>"'. */
    int tmp;
    tmp = connect(s, (struct sockaddr*) &server, sizeof(struct sockaddr_in));
    if (tmp == -1) {
        perror("Connect fails");
        return 1;
    }

// ### Write ###
    /* Nel modello client-server è il client a prendere l'iniziativa.
       Inviamio un request al server utilizzando il protocollo HTTP 0.9
       (si tratta di una Simple-Request).
       Chiediamo il contenuto della radice. Il '\r\n' sta per "carriage return"
       e "new line" (specificato dal protoccollo come CRLF). */
    write(s, "GET /\r\n", 7);

// ### Read ###
    /* Creo un buffer di dim byte in cui salvare il contenuto della pagina.
       Poi leggerò (al massimo) fino a dim - 1 byte in questo modo non rischio 
       di scrivere il terminatore di stringa in 'response[dim]'. */
    const int dim = 100000;
    char response[dim];

    /* * Effetturare una singola lettura *
       Nota che effettuando una singola lettura non ci viene restituita
       l'intera pagina. Stiamo usando un 'SOCK_STREAM' e quindi riceviamo
       un flusso di byte, non dei messaggi. Effettuando una singola lettura
       di fatto si vanno a leggere i byte che fino ad ora sono stati
       inviati dal server e, siccome la CPU è molto più veolce del pacchetto
       di dati che arriva, non facciamo in tempo a rivere l'intera pagina in
       questo modo. */
    //tmp = read(s, response, dim - 1);
    //response[tmp] = 0; // Terminatore di stringa.

    /* Una soluzione sarebbe quella di inserire un'attesa: in questo modo
       si ottiene l'intera pagina web in quanto il server ha avuto tempo
       di inviare tutti i byte. Soluzione poco efficiente. */
    //sleep(2);
    //tmp = read(s, response, dim - 1);
    //response[tmp] = 0; // Terminatore di stringa.

    /* * Ricevere l'intera pagina web *
       Per ricevere l'intera pagina senza dover inserire un'attesa
       ci basta effetturare molteplici chiamate a 'read'. */
    int i = 0;
    for (i = 0; (tmp = read(s, response + i, (dim - 1) - i)); i += tmp) {
        /* Stampa byte ricevuti per ogni chiamata di read.
           Vengono inviati a multipli di 1400 byte: si tratta delle dimensione
           massima del frame ethernet (unità dati trasportata al livello 2). */
        printf("Byte ricevuti: %d\n", tmp);
    }
    response[i] = 0; // Terminatore di stringa.

    /* Stampa pagina web ricevuta. La risposta tornata dal server è una
       Full-Response e non una Simple-Response, ovvero il server utilizza
       il protocollo HTTP 1.0. */
    //printf("%s\n", response);

    // Stampa quanto pesa la pagina web.
    printf("\nDimensione pagina web:  %d byte\n", i);

    return 0;
}
