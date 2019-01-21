#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "interface.h"

using namespace std;

#define SERVER_PORT     3005
#define BUFFER_LENGTH    250
#define NAME_LIMIT        32
#define MAX_CHATROOMS     16
#define FALSE              0
#define DEBUG              0

// struct to store chatroom data
struct chatroom_data {
    int sockfd;
    int port;
    char name[NAME_LIMIT];
    int isActive = 0; // 0 - Inactive, 1 - Active
    struct sockaddr_in serveraddr;
};

// Global Variables
vector<chatroom_data> cdata;
int numChatRooms = 0;
int recentPort = 3005;

/* 0 - Failure, 1 - Success */
int createCommand(char buffer[BUFFER_LENGTH], int sd2) {
    if(numChatRooms < MAX_CHATROOMS) {
        // get the chatroom name
        char subbuff[NAME_LIMIT];
        memcpy(subbuff, &buffer[7], NAME_LIMIT-1);
        subbuff[NAME_LIMIT-1] = '\0';

        // Create the socket and store the file descriptor
        int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
        if (sockfd < 0) 
            perror("ERROR opening socket");

        struct sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family      = AF_INET;
        serveraddr.sin_port        = htons(recentPort+1);
        serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        recentPort++;

        chatroom_data temp;
        temp.sockfd = sockfd;
        temp.port = recentPort;
        strcpy(temp.name, subbuff);
        temp.isActive = 1;
        temp.serveraddr = serveraddr;

        cdata.push_back(temp);
        numChatRooms++;

        printf("Chatroom created Successfully\n");

        char* msgChar = "SUCCESS";
        char msg[BUFFER_LENGTH];
        strcpy(msg, msgChar);

        int rc = send(sd2, msg, sizeof(msg), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }
    } else {
        // TODO: Error Handling
        char* msgChar = "FAILURE";
        char msg[BUFFER_LENGTH];
        strcpy(msg, msgChar);

        int rc = send(sd2, msg, sizeof(msg), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }
    }
    
    return 1;
}

int joinCommand(char buffer[BUFFER_LENGTH], int sd2) {

    return 1;
}

int deleteCommand(char buffer[BUFFER_LENGTH], int sd2) {

    return 1;
}

int listCommand(char buffer[BUFFER_LENGTH], int sd2) {
    // Check if there are any chatrooms
    if(numChatRooms > 0) {
        std::string msgChar = "";

        for(int i = 0; i < cdata.size(); i++) {
            if(cdata.at(i).isActive == 1) {
                std::string tp(cdata[i].name);
                msgChar += tp;
            }
        }

        char msg[BUFFER_LENGTH];
        strcpy(msg, msgChar.c_str());   

        // int loop = 0;
        // while(msg[loop] != '\0') {
        //     printf("%c", msg[loop]);
        //     loop++;
        // }
        // printf("\n");

        int rc = send(sd2, msg, sizeof(msg), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }

    } else {
        printf("No Active Chat Rooms...\n");

        char* msgChar = "No Active Chat Rooms";
        char msg[BUFFER_LENGTH];
        strcpy(msg, msgChar);

        int rc = send(sd2, msg, sizeof(msg), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }

    }
    return 1;
}

int identifyCommand(char buffer[BUFFER_LENGTH], int sd2) {

    if(strncmp(buffer, "JOIN", 4) == 0) {
        joinCommand(buffer, sd2);
    }
    else if(strncmp(buffer, "CREATE", 6) == 0) {
        createCommand(buffer, sd2);
    }
    else if(strncmp(buffer, "DELETE", 6) == 0) {
        deleteCommand(buffer, sd2);
    }
    else if(strncmp(buffer, "LIST", 4) == 0) {
        listCommand(buffer, sd2);
    }
    else {
        printf("Unknown Command\n");
        return 0;
    }
    return 1;
}

int main() {
   int    sd=-1, sd2=-1;
   int    rc, length, on=1;
   char   buffer[BUFFER_LENGTH];
   fd_set read_fd;
   struct timeval timeout;
   struct sockaddr_in serveraddr;

    do {
        sd = socket(AF_INET, SOCK_STREAM, 0);
        if(sd < 0) {
            perror("Failed to create socket");
        }   

        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family      = AF_INET;
        serveraddr.sin_port        = htons(SERVER_PORT);
        serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        rc = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to bind socket");
        }  

        rc = listen(sd, 10);
        // test error rc< 0
        if(rc < 0) {
            perror("Failed to listen on socket");
        }   

        printf("Ready for client connect().\n");

        for(;;) {
            sd2 = accept(sd, NULL, NULL);
            // test error sd2 < 0
            if(sd2 < 0) {
                perror("Failed to accept socket");
            }   

            timeout.tv_sec  = 30;
            timeout.tv_usec = 0;

            FD_ZERO(&read_fd);
            FD_SET(sd2, &read_fd);

            rc = select(sd2+1, &read_fd, NULL, NULL, &timeout);
            // test error rc < 0
            if(rc < 0) {
                perror("Failed to select socket");
            }   

            length = BUFFER_LENGTH;

            if (DEBUG) {
                printf("Attempting to recieve msg\n");
            }
            rc = recv(sd2, buffer, sizeof(buffer), 0);
            // test error rc < 0 or rc == 0 or   rc < sizeof(buffer
            if(rc < 0) {
                perror("Failed to recieve on socket");
            }   

            identifyCommand(buffer, sd2);

            if (DEBUG) {
                printf("Printing...\n");

                int loop = 0;
                while(buffer[loop] != '\0') {
                    printf("%c", buffer[loop]);
                    loop++;
                }

                printf("\n");
            }
        }

    } while (FALSE);

    if (sd != -1)
    close(sd);
    if (sd2 != -1)
    close(sd2);

    return 0;
}

