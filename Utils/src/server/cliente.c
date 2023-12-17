#include "cliente.h"



int conectar_a_servidor(char* ip, char* port) {
    int conn;
    struct addrinfo* hints = malloc(sizeof(*hints));
    struct addrinfo* serverInfo = malloc(sizeof(*serverInfo));
    struct addrinfo* p = malloc(sizeof(*p));

    memset(hints, 0, sizeof(*hints));
    	hints->ai_family = AF_INET;
    	hints->ai_socktype = SOCK_STREAM;

    int rv = getaddrinfo(ip, port, hints, &serverInfo);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo error: %s", gai_strerror(rv));
        return EXIT_FAILURE;
    }
    for (p = serverInfo; p != NULL; p = p->ai_next) {
        conn = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (-1 == conn) {
            continue;
        }
        if (connect(conn, p->ai_addr, p->ai_addrlen) != -1) {
            break;
        }
        close(conn);
    }
    free(serverInfo);
    free(hints);
    //freeaddrinfo(hints);
    if (conn != -1 && p != NULL) {
        return conn;
    }
    freeaddrinfo(p);
    
    return -1;
}
