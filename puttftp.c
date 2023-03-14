#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SIZE 512


//IMPORTANT
//La syntaxe est comme suit: ./monprogramme mon adresse IP, mon port , le nom du fichier
int main(int argc, char **argv) {
    int sockfd, n;
    socklen_t serverlen;
    struct sockaddr_in serveraddr;
    unsigned char buf[MAX_SIZE];
    int myport= atoi(argv[2]);

    if (argc != 4) {
        printf("Nombre d'arguments incorrect\n ");
        printf("Le format correct est le suivant :");
        printf("./monprogramme mon adresse IP mon port le nom du fichiert :");
        exit(1);
    }

    //Création de la socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        printf("Erreur lors de la création de la socket");
        exit(1);
    }

    //On récupère l'IP du serveur
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(myport);
    serverlen = sizeof(serveraddr);

    //Ouverture de notre fichier
    FILE *myfile = fopen(argv[3], "rb");
    if(myfile == NULL) {
        printf("Error opening file\n");
        exit(1);
    }

    //Construction et envoi des nos WRQs
    unsigned char myWRQ[MAX_SIZE];
    myWRQ[0] = 0;
    myWRQ[1] = 2;
    int filename_len = strlen(argv[3]);
    memcpy(myWRQ + 2, argv[3], filename_len);
    myWRQ[filename_len + 2] = 0;
    const char *mode = "octet";
    int mode_len = strlen(mode);
    memcpy(myWRQ + filename_len + 3, mode, mode_len);
    myWRQ[filename_len + mode_len + 3]    = 0;
    n = sendto(sockfd, myWRQ, filename_len + mode_len + 4, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (n < 0) {
        printf("Erreur lors de l''expédition");
        exit(1);
    }


    n = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
    if (n < 0) {
        printf("Erreur de réception");
        exit(1);
    }
    if(buf[1] != 4) {
        printf("WRQs non reconnues par le serveur");
        exit(1);
    }

    //Expédition du fichier
    int block_number = 1;
    while(1) {
        //On lit le fichier
        n = fread(buf + 4, 1, MAX_SIZE - 4, myfile);
        if(n <= 0) {
            break;
        }

        //On construit et expédie notre bloc de données
        buf[0] = 0;
        buf[1] = 3;
        buf[2] = (block_number >> 8) & 0xFF;
        buf[3] = block_number & 0xFF;
        n = sendto(sockfd, buf, n + 4, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        if (n < 0){
            printf("Erreur d'expédition des WRQs");
            exit(1);
        }
        //On attend la réponse sous forme d'ACKs
        n = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
        if (n < 0){
            printf("Erreur de réception\n");
            exit(1);
        }
        if(buf[1] != 4) {
            printf("Error: Data block not acknowledged by server\n");
            exit(1);
        }

        block_number++;
    }

    fclose(myfile);
    close(sockfd);
    return 0;
}
