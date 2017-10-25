

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "needs.h"

//struct logged_in_user* users;
//struct session* sessions;

#define STDIN 0

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char* argv[]) {

    char receiving_buffer[1024];


    if (argc != 1) {
        printf("./client\n\n");

    }

    char command[MAX_DATA];
    char client_id[MAX_DATA];
    char password[MAX_DATA];
    char server_ip[MAX_DATA];
    char session_id[MAX_DATA];
    char port_num_char[MAX_DATA];
    char to_client[MAX_DATA];
    char uni_msg[MAX_DATA];

    struct addrinfo hints, *servinfo, *p;

    char serv[INET6_ADDRSTRLEN];

    int client_socket, activity;
    struct sockaddr_in server_address;
    socklen_t address_size;
    int port_number;


    int login = 0;
    int in_session = 0;

    fd_set read_fds;

    FD_ZERO(&read_fds);

    FD_SET(STDIN, &read_fds); //



    while (!login) {

        printf("Please log in:\n");
        fgets(receiving_buffer, sizeof (receiving_buffer), stdin);

        sscanf(receiving_buffer, "%s", command);

        if (strcmp(command, "/login") == 0) {
            sscanf(receiving_buffer, "%s %s %s %s %s", command, client_id, password, server_ip, port_num_char);




            // create socket and connect to the server
            int rv;


            memset(&hints, 0, sizeof hints);
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_STREAM;

            if ((rv = getaddrinfo(server_ip, port_num_char, &hints, &servinfo)) != 0) {
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                return 1;
            }

            // CREATING SOCKET + CONNECTING TO THE SERVER
            // loop through all the results and connect to the first we can
            for (p = servinfo; p != NULL; p = p->ai_next) {
                if ((client_socket = socket(p->ai_family, p->ai_socktype,
                        p->ai_protocol)) == -1) {
                    perror("client: socket");
                    continue;
                }
                printf("SOCKET MADE: %d\n\n", client_socket);
                if (connect(client_socket, p->ai_addr, p->ai_addrlen) == -1) {
                    perror("client: connect");
                    close(client_socket);
                    continue;
                }

                break;
            }

            if (p == NULL) {
                fprintf(stderr, "client: failed to connect\n");
                return 2;
            }

            inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
                    serv, sizeof serv);
            printf("client: connecting to %s\n", serv);

            freeaddrinfo(servinfo);


            struct lab3message msg;
            struct lab3message* msg_ptr = &msg;


            // make my pckage
            msg_ptr->type = LOGIN;
            msg_ptr->size = sizeof (receiving_buffer);
            strcpy(msg_ptr->data, receiving_buffer);
            strcpy(msg_ptr->source, client_id);


            send(client_socket, msg_ptr, sizeof (struct lab3message), 0);


            recv(client_socket, msg_ptr, sizeof (struct lab3message), 0);


            if (msg_ptr->type == LO_ACK) { // successful login
                login = 1;
                printf("You have successfully logged in!\n");
            } else if (msg_ptr->type == LO_NAK) {
                printf("Error: Authentication\n");
            } else
                printf("No.\n");




            login = 1;
        } else {
            printf("You must be logged in first!\n");
        }
    }





    // initialize when they do login 





    while (1) {

        FD_ZERO(&read_fds);
        FD_SET(STDIN, &read_fds);
        FD_SET(client_socket, &read_fds);


        select(client_socket + 1, &read_fds, NULL, NULL, NULL);

        
        
        if (FD_ISSET(STDIN, &read_fds)) { //see if the STDIN is ready 
            fgets(receiving_buffer, sizeof (receiving_buffer), stdin);

            sscanf(receiving_buffer, "%s", command);
            //            parse_buffer(command_checking, receiving_buffer);



            if (receiving_buffer[0] == '/') {

                if (strcmp(command, "/login") == 0) {


                    if (login == 0) { // if not logged in 
                        sscanf(receiving_buffer, "%s %s %s %s %s", command, client_id, password, server_ip, port_num_char);

                        // create socket and connect to the server
                        int rv;

                        memset(&hints, 0, sizeof hints);
                        hints.ai_family = AF_UNSPEC;
                        hints.ai_socktype = SOCK_STREAM;

                        if ((rv = getaddrinfo(server_ip, port_num_char, &hints, &servinfo)) != 0) {
                            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
                            return 1;
                        }

                        // loop through all the results and connect to the first we can
                        for (p = servinfo; p != NULL; p = p->ai_next) {
                            if ((client_socket = socket(p->ai_family, p->ai_socktype,
                                    p->ai_protocol)) == -1) {
                                perror("client: socket");
                                continue;
                            }
                            printf("SOCKET MADE: %d\n\n", client_socket);
                            if (connect(client_socket, p->ai_addr, p->ai_addrlen) == -1) {
                                perror("client: connect");
                                close(client_socket);
                                continue;
                            }

                            break;
                        }

                        if (p == NULL) {
                            fprintf(stderr, "client: failed to connect\n");
                            return 2;
                        }

                        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *) p->ai_addr),
                                serv, sizeof serv);
                        printf("client: connecting to %s\n", serv);

                        freeaddrinfo(servinfo);


                        struct lab3message msg;
                        struct lab3message* msg_ptr = &msg;


                        // make my pckage
                        msg_ptr->type = LOGIN;
                        msg_ptr->size = sizeof (receiving_buffer);
                        strcpy(msg_ptr->data, receiving_buffer);
                        strcpy(msg_ptr->source, client_id);


                        send(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                        recv(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                        if (msg_ptr->type == LO_ACK) { // successful login
                            login = 1;
                            printf("You have successfully logged in!\n\n");
                        } else if (msg_ptr->type == LO_NAK) {
                            printf("Error: Authentication.\n\n");
                        } else {
                            printf("No.\n\n");
                        }
                        
                        login = 1;
                        
                    }
                    else {
                        printf("You are logged in already!\n\n");
                    }
                }


                else if (strcmp(command, "/logout") == 0) {
                    if (in_session == 0){
                        struct lab3message msg;
                        struct lab3message* msg_ptr = &msg;

                        msg_ptr->type = EXIT;
                        strcpy(msg_ptr->source, client_id);

                        send(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                        in_session = 0;
                        login = 0;

                        close(client_socket);
                    }
                    else {
                        printf("You must leave your session first!\n\n");
                    }


                } else if (strcmp(command, "/joinsession") == 0) {
                    // parse
                    sscanf(receiving_buffer, "%s %s", command, session_id);

                    struct lab3message msg;
                    struct lab3message* msg_ptr = &msg;

                    if (login == 1 && in_session == 0) {

                        strcpy(msg_ptr->source, client_id);
                        msg_ptr->type = JOIN;
                        strcpy(msg_ptr->data, receiving_buffer);
                        msg_ptr->size = sizeof (receiving_buffer);

                        send(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                        recv(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                        if (msg_ptr->type == JN_ACK) {
                            printf("You've successfully joined: %s!\n", session_id);
                            in_session = 1;
                        } else if (msg_ptr->type == JN_NAK) {
                            printf("You've failed to join: %s\n", session_id);
                        } else {
                            printf("No.\n");
                        }
                    }
                    else {
                        if (login == 0)
                            printf("Not logged in\n");
                        else
                            printf("You've already joined another session!\n");
                    }

                }// end of join session

                else if (strcmp(command, "/leavesession") == 0) {

                    struct lab3message msg;
                    struct lab3message* msg_ptr = &msg;

                    msg_ptr->type = LEAVE_SESS;
                    strcpy(msg_ptr->source, client_id);

                    send(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                    in_session = 0;

                }
                else if (strcmp(command, "/createsession") == 0) {
                    sscanf(receiving_buffer, "%s %s", command, session_id);

                    struct lab3message msg;
                    struct lab3message* msg_ptr = &msg;

                    if (login == 1 && in_session == 0) {
                        strcpy(msg_ptr->source, client_id);
                        msg_ptr->type = NEW_SESS;
                        strcpy(msg_ptr->data, receiving_buffer);
                        msg_ptr->size = sizeof (receiving_buffer);

                        send(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                        recv(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                        if (msg_ptr->type == NS_ACK) {
                            printf("You've successfully created: %s!\n", session_id);
                            in_session = 1;
                        } else if (msg_ptr->type == NS_NAK) {
                            printf("Already created the session: %s\n", session_id);
                        } else {
                            printf("No.\n");
                        }
                    } else {
                        if (login == 0) {
                            printf("You aren't logged in!\n");
                        } else {
                            printf("You are already in a session!\n");
                        }
                    }


                } else if (strcmp(command, "/list") == 0) {

                    struct lab3message msg;
                    struct lab3message* msg_ptr = &msg;

                    msg_ptr->type = QUERY; // set it to msg type
                    strcpy(msg_ptr->data, receiving_buffer);
                    strcpy(msg_ptr->source, client_id);
                    msg_ptr->size = sizeof (receiving_buffer);

                    send(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                    recv(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                    if (msg_ptr->type == QU_ACK) {
                        printf("%s\n", msg_ptr->data);
                    } else {
                        printf("Error: Not logged in.\n");
                    }
                }
                else if (strcmp(command, "/quit") == 0) {
                    close(client_socket);
                    exit(1);

                }
                else if (strcmp(command, "/unicast") == 0) { // our special feature 

                    if ((sscanf(receiving_buffer, "%s %s", command, to_client)) == 2) {
                        struct lab3message msg;
                        struct lab3message* msg_ptr = &msg;


                        msg_ptr->type = UNIMSG;
                        msg_ptr->size = sizeof (receiving_buffer);
                        strcpy(msg_ptr->source, client_id);
                        strcpy(msg_ptr->data, receiving_buffer);

                        send(client_socket, msg_ptr, sizeof (struct lab3message), 0);


                        printf("\nType in your message you want to send to %s:\n", to_client);
                        fgets(receiving_buffer, sizeof (receiving_buffer), stdin);

                        send(client_socket, receiving_buffer, sizeof (receiving_buffer), 0);

                    } else {
                        printf("WRONG NUMBER OF ARGUMENTS\n\n");
                    }
                }
            } else { // broadcast 
                if (login == 1 && in_session == 1) {

                    struct lab3message msg;
                    struct lab3message* msg_ptr = &msg;

                    msg_ptr->type = MESSAGE; // set it to msg type
                    strcpy(msg_ptr->data, receiving_buffer);
                    strcpy(msg_ptr->source, client_id);
                    msg_ptr->size = sizeof (receiving_buffer);

                    send(client_socket, msg_ptr, sizeof (struct lab3message), 0);

                } else {
                    if (login == 0) {
                        printf("Not logged in.\n");
                    }
                    else {
                        printf("Not in session.\n");
                    }
                }
            }
        }// i think error is here 


        else if (FD_ISSET(client_socket, &read_fds)) {
            int n;
            struct lab3message asd;
            struct lab3message* message = &asd;

            n = recv(client_socket, message, sizeof (struct lab3message), 0);

            if (message == NULL) {
                printf("What's wrong...\n");
            } else if (message->type == MESSAGE) {
                printf("%s", message->data);
            } else if (message->type == UNIMSG) {
                printf("%s", message->data);
            } else {
                ;
            }
        }

    }


    return 0;
}

