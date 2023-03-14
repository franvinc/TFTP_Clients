#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_SIZE 512

//IMPORTANT
//La syntaxe est comme suit: ./monprogramme mon adresse IP, mon port , le nom du fichier
int main(int argc, char **argv) {
    int sockfd, res;
    socklen_t serverlen;
    struct sockaddr_in serveraddr;
    unsigned char buf[MAX_SIZE];

    if (argc != 4) {
        printf("Nombre d'arguments incorrect\n ");
        printf("Le format correct est le suivant :");
        printf("./monprogramme mon adresse IP mon port le nom du fichiert :");
        exit(1);
    }

    //Création de la socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0){
        printf("Erreur lors de la création de la socket \n");
        exit(1);
    }


    //Configuration de l'IP et du port
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    int myport= atoi(argv[2]);
    serveraddr.sin_port = htons(myport);

    //Construction et envoi des RRQ
    unsigned char myRRQ[MAX_SIZE];
    myRRQ[0] = 0;
    myRRQ[1] = 1;
    int filename_len = strlen(argv[3]);
    memcpy(myRRQ + 2, argv[3], filename_len);
    myRRQ[filename_len + 2] = 0;
    const char *mode = "octet";
    int mode_len = strlen(mode);
    memcpy(myRRQ + filename_len + 3, mode, mode_len);
    myRRQ[filename_len + mode_len + 3] = 0;
    res = sendto(sockfd, myRRQ, filename_len + mode_len + 4, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (res < 0){
        printf("Problème d'expédition \n");
        exit(1);
    }

    //Réception de notre fichier
    unsigned int block_number = 1;
    while(1) {
        serverlen = sizeof(serveraddr);

        res = recvfrom(sockfd, buf, MAX_SIZE, 0, (struct sockaddr *)&serveraddr, &serverlen);
        if (res < 0){
            printf("Problème de réception \n");
            exit(1);
        }


        // On cherche la fin de notre fichier
        if (res < MAX_SIZE) {
            FILE *myfile = fopen(argv[3], "wb"); // On ouvre le fichier en mode d'écriture binaire
            if(myfile == NULL) {
                printf("Erreur lors de l'ouverture du fichier\n");
                exit(1);
            }
            fwrite(buf, 1, res, myfile);
            fclose(myfile);
            break;
        }

        // On écrit nos données dans le fichier
        FILE* myfile = fopen(argv[3], "ab");
        if(myfile == NULL) {
            printf("Erreur lors de l'ouverture du fichier\n");
            exit(1);
        }
        fwrite(buf, 1, res, myfile);
        fclose(myfile);

        //Construction et envoi des ACKs
        unsigned char myACK[4];
        myACK[0] = 0;
        myACK[1] = 4;
        myACK[2] = (block_number >> 8) & 0xFF;//
        myACK[3] = block_number & 0xFF;
        res = sendto(sockfd, myACK, sizeof(myACK), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        if (res < 0){
            printf("Erreur lors de l'envoi des ACKs\n");
            exit(1);
        }


        block_number++;
    }

    close(sockfd);
    return 0;
}
