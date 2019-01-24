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

#define SERVER_PORT     3005
#define SERVER_ADDR     "127.0.0.1"
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
std::vector<chatroom_data> cdata;
std::vector<int> availablePorts;

/* 0 - Failure, 1 - Success */

// CREATE
int createCommand(char buffer[BUFFER_LENGTH], int sd2) {
    if(cdata.size() < MAX_CHATROOMS) { // Can allow more chatrooms
        // Get the chatroom name
        char subbuff[NAME_LIMIT];
        memcpy(subbuff, &buffer[7], NAME_LIMIT-1);
        subbuff[NAME_LIMIT-1] = '\0';

        // Convert name to string
        std::string str(subbuff);
        // Check if chatroom already exists
        for(int i = 0; i < cdata.size(); i++) {
            std::string name(cdata.at(i).name);
            if(str == name) {
                std::string msg = "FAILURE_ALREADY_EXISTS";
                char msgChar[BUFFER_LENGTH];
                strcpy(msgChar, msg.c_str());

                int rc = send(sd2, msgChar, sizeof(msgChar), 0);
                // test error rc < 0
                if(rc < 0) {
                    perror("Failed to reply: CREATE Command");
                }

                printf("Chatroom name already exists\n");

                return 0;
            }
        }

        /* Chatroom creation was sucessful */ 
        // Create the socket and slave port
        int sockfd = socket(AF_INET, SOCK_STREAM, 0); 
        if (sockfd < 0) 
            perror("ERROR opening socket");

        // Find an available port and delete it
        int port = availablePorts.at(0);
        availablePorts.erase(availablePorts.begin());

        struct sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family      = AF_INET;
        serveraddr.sin_port        = htons(port);
        serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

        chatroom_data temp;
        temp.sockfd = sockfd;
        temp.port = port;
        strcpy(temp.name, subbuff);
        temp.isActive = 1;
        temp.serveraddr = serveraddr;

        cdata.push_back(temp);

        printf("Chatroom created Successfully\n");

        std::string msg = "SUCCESS";
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());

        int rc = send(sd2, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: CREATE Command");
        }

    } else {
        // TODO: Error Handling
        std::string msg = "FAILURE_INVALID";
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());

        int rc = send(sd2, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: CREATE Command");
        }

        printf("Max chatrooms already created\n");
    }
    
    return 1;
}

// JOIN
int joinCommand(char buffer[BUFFER_LENGTH], int sd2) {

    return 1;
}

// DELETE
int deleteCommand(char buffer[BUFFER_LENGTH], int sd2) {
    // Initialize reply to failure
    std::string msgFail = "FAILURE_NOT_EXISTS";
    char msgFailChar[BUFFER_LENGTH];
    strcpy(msgFailChar, msgFail.c_str());

    // check if there are any chatrooms to even delete
    if(cdata.size() == 0) {
        int rc = send(sd2, msgFailChar, sizeof(msgFailChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: DELETE Command");
        }
        printf("No chatrooms exist\n");
        return 0;
    }

    // Get the chatroom name
    char subbuff[NAME_LIMIT];
    memcpy(subbuff, &buffer[7], NAME_LIMIT-1);
    subbuff[NAME_LIMIT-1] = '\0';
    // Convert name to string
    std::string str(subbuff);

    for(int i = 0; i < cdata.size(); i++) {
        std::string name(cdata.at(i).name);
        if(str == name) {
            int port = cdata.at(i).port;

            // Close the connection
            close(cdata.at(i).sockfd);
            // Remove the chat room from the list
            cdata.erase(cdata.begin() + i);

            // Make the port available again
            availablePorts.push_back(port);

            std::string msg = "SUCCESS";
            char msgChar[BUFFER_LENGTH];
            strcpy(msgChar, msg.c_str());

            int rc = send(sd2, msgChar, sizeof(msgChar), 0);
            // test error rc < 0
            if(rc < 0) {
                perror("Failed to reply: CREATE Command");
            }
            printf("Chatroom deleted successfully\n");
            return 1;
        }
    }

    int rc = send(sd2, msgFailChar, sizeof(msgFailChar), 0);
    // test error rc < 0
    if(rc < 0) {
        perror("Failed to reply: DELETE Command");
    }
    printf("Chatroom of that name does not exist\n");
    return 0;
}

// LIST
int listCommand(char buffer[BUFFER_LENGTH], int sd2) {
    // Check if there are any chatrooms
    if(cdata.size() > 0) {
        // Build reply string
        std::string msg = "";
        for(int i = 0; i < cdata.size(); i++) {
            if(cdata.at(i).isActive == 1) {
                std::string tp(cdata[i].name);
                if(i < cdata.size()-1) { msg += tp + ","; }
                else { msg += tp; }
            }
        }
        
        // Convert back to char[]
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());   

        printf("LIST: ");
        int loop = 0;
        while(msgChar[loop] != '\0') {
            printf("%c", msgChar[loop]);
            loop++;
        }
        printf("\n");

        int rc = send(sd2, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }

    } else {
        printf("No Active Chat Rooms...\n");

        std::string msg = "No Active Chat Rooms";
        char msgChar[BUFFER_LENGTH];
        strcpy(msgChar, msg.c_str());

        int rc = send(sd2, msgChar, sizeof(msgChar), 0);
        // test error rc < 0
        if(rc < 0) {
            perror("Failed to reply: LIST Command");
        }

    }
    return 1;
}

int identifyCommand(char buffer[BUFFER_LENGTH], int sd2) {

    if(strncmp(buffer, "JOIN ", 5) == 0) {
        joinCommand(buffer, sd2);
    }
    else if(strncmp(buffer, "CREATE ", 7) == 0) {
        createCommand(buffer, sd2);
    }
    else if(strncmp(buffer, "DELETE ", 7) == 0) {
        deleteCommand(buffer, sd2);
    }
    else if(strncmp(buffer, "LIST", 4) == 0) {
        listCommand(buffer, sd2);
    }
    else {
        // Error Handling:
        // no need to send b/c already handled on client end
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

    // Populate available ports w/ 3006->3022
    for(int i = 0; i < MAX_CHATROOMS; i++) {
        availablePorts.push_back(3006+i);
    }

    do {
        sd = socket(AF_INET, SOCK_STREAM, 0);
        // test error sd< 0
        if(sd < 0) {
            perror("Failed to create socket");
        }   

        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family      = AF_INET;
        serveraddr.sin_port        = htons(SERVER_PORT);
        serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

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

