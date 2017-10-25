
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
#include "server.h"
#include "needs.h"


/* GLOBAL VARIABLES AND STRUCTURES */
// structures 
struct logged_in_user* users;
struct session* sessions;



// variables 
fd_set master; // master file descriptor list
int fdmax; // maximum file descriptor number

/* Base of this server code from Beej*/


// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*) sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*) sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
    
    users = NULL; // they are pointing to nothing at the beginning 
    sessions = NULL;
    
    if (argc != 2){
        printf("Error: server <port_number>\n\n");
        exit(1);
    }        


    fd_set read_fds; // temp file descriptor list for select()
    
    int listener; // listening socket descriptor
    int newfd; // newly accept()ed socket descriptor
    
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    
    char buffer[1024]; // buffer for client data
    int nbytes = 0;
    char remoteIP[INET6_ADDRSTRLEN];
    int yes = 1; // for setsockopt() SO_REUSEADDR, below
    int i, j, rv;
    struct addrinfo hints, *ai, *p;
    
    
    // my variables 
    int port_number;
    struct lab3message message; 
    
    char server_port_number[MAX_DATA];

    strcpy(server_port_number, argv[1]);
    

    
    // get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    // argv[1] == the port number but this is char*
    if ((rv = getaddrinfo(NULL, server_port_number, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
   
    for (p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) {
            continue;
        }

        // lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof (int));
        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }
        break;
    }

    // if we got here, it means we didn't get bound
    if (p == NULL) {
        fprintf(stderr, "selectserver: failed to bind\n");
        exit(2);
    }
    
    freeaddrinfo(ai); // all done with this
    
    
    // listen
    if (listen(listener, 10) == -1) {
        perror("listen");
        exit(3);
    }
    
    
    FD_ZERO(&master); // clear the master and temp sets
    FD_ZERO(&read_fds);
    // add the listener to the master set
    FD_SET(listener, &master);
    
    // keep track of the biggest file descriptor
    fdmax = listener; // so far, it's this one
    // main loop
    while(1) {
        read_fds = master; // copy it
        
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        

        // run through the existing connections looking for data to read
        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == listener) { // if it's the new connection 
                    // handle new connections
                    
                    addrlen = sizeof remoteaddr;
                    
                    
                    newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);

                    if (newfd == -1) {
                        
                        perror("accept");
                    } 
                    else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) { // keep track of the max
                            fdmax = newfd;
                        }
                        printf("selectserver: new connection from %s on socket %d\n", inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*) &remoteaddr), remoteIP, INET6_ADDRSTRLEN), newfd);
                    }
                    
                                                                                                 
                    
                } 
                
                else {
                    struct lab3message* message_ptr = &message;
                    
                    // handle data from a client
                    if ((nbytes = recv(i, message_ptr, sizeof(struct lab3message), 0)) <= 0) {// got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } 
                        else {
                            perror("recv");
//                            printf("Waht the fuck?\n");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    } 
                    else {
                        // message_ptr is pointing to the message we received 
                        
                        get_message_type(message_ptr, i);

                        
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!

    return 0;
}



void get_message_type(struct lab3message* msg, int fd){
    
    
    switch(msg -> type){
        case(LOGIN):
            login_conference(msg, fd);
            break;
            
        case(EXIT):
            log_out_conference(msg, fd);
            break;
        
        case(JOIN):
            join_session(msg, fd);
            break;


        case(LEAVE_SESS):
            leave_session(msg, fd);
            break;
            
        case(NEW_SESS):
            new_session(msg, fd);
            break;
            
        case(MESSAGE):
            broadcast(msg, fd);
            break;
            
        case(QUERY):
            list(msg, fd);
            break;
            
        case(UNIMSG):
            unicast(msg, fd);
            break;

            
    }
}

void login_conference(struct lab3message* msg, int fd){
    // make a structure that we will be using in our LOGIN STRUCTURE 
    struct logged_in_user* user = (struct logged_in_user*)malloc(sizeof(struct logged_in_user) );
    
    char command[MAX_DATA];
    char client_id[MAX_DATA];
    char password[MAX_DATA];
    char server_ip[MAX_DATA];
    int server_port;

    // parse through the message and set up the logged_in_user structure 
    sscanf(msg->data, "%s %s %s %s %d", command, client_id, password, server_ip, &server_port);
    
    strcpy(user->password, password);
    strcpy(user->client_id, client_id);
    
    user->fd = fd;
    //user->joined_session = 0; // not yet joined 
    user->next = NULL;
    user->session_id[0] = '\0'; // not joined a session

    /* Authenticate */
    int check = 0;
    
    /*********************** Out hard coded client_id and password "database" ******************************/
    if (strcmp(client_id, "cow") == 0){
        if (strcmp(password, "cow") == 0){
//            int check_if_logged_in = 0;
            
            // look through the list of logged in users to see if logged in or not 
            
            struct logged_in_user* check_if_logged_in = search_user_list(client_id);
            
            
            if (check_if_logged_in == NULL){// not logged in --> so yes, log them in 
                check = 1;
            }
            
            else {
                printf("Already logged in.\n\n");
            }
 
        }
    }
    
    else if (strcmp(client_id, "moo") == 0){
        if (strcmp(password, "moo") == 0){
            
            struct logged_in_user* check_if_logged_in = search_user_list(client_id);
            
            
            if (check_if_logged_in == NULL){// not logged in --> so yes, log them in 
                check = 1;
            }
            
            else {
                printf("Already logged in.\n\n");
            }
            
  
        }
    }
    
    else if (strcmp(client_id, "moon") == 0){
        if (strcmp(password, "moon") == 0){
            
            struct logged_in_user* check_if_logged_in = search_user_list(client_id);
            
            
            if (check_if_logged_in == NULL){// not logged in --> so yes, log them in 
                check = 1;
            }
            
            else {
                printf("Already logged in.\n\n");
            }
        }
    }
    
    else if (strcmp(client_id, "joon") == 0){
        if (strcmp(password, "joon") == 0){

            struct logged_in_user* check_if_logged_in = search_user_list(client_id);
            
            
            if (check_if_logged_in == NULL){// not logged in --> so yes, log them in 
                check = 1;
            }
            
            else {
                printf("Already logged in.\n\n");
            }
        }
    }
    
    else {
        check = 0;
    }
    /*************************************************************************************************/
    
    // just showing messages for debugging purposes and constructing the acknowledge message 
    struct lab3message ack;
    struct lab3message* ack_ptr = &ack;
    
    
    if (check == 0){

        ack_ptr->type = LO_NAK;
        
        
        printf("Error: authenticating %s\n\n", client_id);
    }
    else{ // successful log in 
        
        
        ack_ptr->type = LO_ACK;
        
        // add it to the users list 
        if (users == NULL){
            users = user; // users = list, user = the new struct made in the beginning of the function
            
        }
        else {
            user->next = users; // add to the beginning of the array 
            users = user;
        }
       
        printf("Successfully logged in %s\n\n", client_id);
    }
    
    send(fd, ack_ptr, sizeof(struct lab3message), 0);

}

void log_out_conference(struct lab3message* msg, int fd){ // logout 
    
    char client_id[MAX_DATA];
    
    strcpy(client_id, msg->source);

    // delete the structure from the user list 

    // corner: take care of the first entry case, b/c seg fault
    if (users != NULL){
        struct logged_in_user* cur = users->next; // the previous if case makes sure this isn't a seg fault
        struct logged_in_user* prev = users;
        

        // checking the first element
        if (strcmp(client_id, users->client_id) == 0){
            struct logged_in_user* temp = users; 
            
            users = users->next;
            printf("DEBUG1\n");
            free(temp);
        }
        
        else { // checking the users in the middle of the list 

            while (cur != NULL){
                if( strcmp(cur->client_id, client_id) == 0 ){
                    struct logged_in_user* temp = cur;
                    prev->next = cur->next;

                    free(cur); // deleted ----> logged out 
                    cur = NULL;
                }

                else {
                    prev = cur; 
                    cur = cur->next;
                }
            }
            
            
        }
    }

    FD_CLR(fd, &master);
    
    
}

void join_session(struct lab3message* msg, int fd){
    
    char command[MAX_DATA];
    char session_id[MAX_DATA];
    char client_id[MAX_DATA];
    
    strcpy(client_id, msg->source);
    
    struct logged_in_user* check_if_logged_in = search_user_list(client_id);
    
    struct lab3message ack;
    struct lab3message* ack_ptr = &ack;
    
    if (check_if_logged_in == NULL){ // user not logged in 
        ack_ptr->type = JN_NAK;
        strcpy(ack_ptr->data, "You aren't logged in!\n");
        
        send(fd, ack_ptr, sizeof(struct lab3message), 0); // send the NACK
    }
    
    else {
        // now check if the session exists or not
        sscanf(msg->data, "%s %s", command, session_id);
        
        struct session* existing_session = search_session_list(session_id);
        
        if (existing_session == NULL){
            ack_ptr->type = JN_NAK;
            strcpy(ack_ptr->data, "No such session!\n");
        
            send(fd, ack_ptr, sizeof(struct lab3message), 0); // send the NACK
        }
        else if(existing_session != NULL){ 
            // the session exists
            
            // change the user's session info 
           
            strcpy(check_if_logged_in->session_id, session_id);
            
            // forge the message being sent to the client
            ack_ptr->type = JN_ACK;
            strcpy(ack_ptr->data, "You joined the session!\n");
            
            struct logged_in_user* put_info = search_user_list(client_id);
        
            if(put_info != NULL){
                strcpy(put_info->session_id, session_id);
            }
            
            send(fd, ack_ptr, sizeof(struct lab3message), 0); // send the ACK
        }
        else 
            return;
    }
    
}

void leave_session(struct lab3message* msg, int fd){
    
    // delete
    char client_id[MAX_DATA];
    char session_id[MAX_DATA];
    
    strcpy(client_id, msg->source);
    
    struct logged_in_user* source = search_user_list(client_id);
    
    if (source == NULL) {
        printf("Not logged in.\n");
        return;
    }
    
    strcpy(session_id, source->session_id);
    
    struct session* leaving = search_session_list(session_id);
    
    if (leaving == NULL){
        printf("No such session.\n");
        return;
    }
    
    else if ( strcmp(source->session_id, session_id) == 0){

        
        source->session_id[0] = '\0';
        
        
        struct logged_in_user* temp = users; 
        int counter = 0;
        
        
        if (temp != NULL) {
            while (temp != NULL){
                if( strcmp(temp->session_id, session_id) == 0 ){
                    counter++;
                }
                temp = temp->next;
            }

            
            // only if it was the only one in the session
            if (counter == 0){
                // delete from the
                if (sessions != NULL){
                    struct session* cur = sessions->next; // the previous if case makes sure this isn't a seg fault
                    struct session* prev = sessions;


                    // checking the first element
                    if (strcmp(session_id, sessions->session_id) == 0){
                        struct session* s = sessions; 

                        sessions = sessions->next;

                        free(temp);
                        
                    }

                    else { // checking the users in the middle of the list 

                        while (cur != NULL){
                            if( strcmp(cur->session_id, session_id) == 0 ){
                                struct session* temp_session = cur;
                                prev->next = cur->next;

                                free(cur); // deleted ----> logged out 
                                cur = NULL;
                            }
                            else {
                                prev = cur; 
                                cur = cur->next;
                            }
                        }
                    }
                }
            }

        }
        
        
    }
    
    
    

}

void new_session(struct lab3message* msg, int fd){
    
    char command[MAX_DATA];
    char session_id[MAX_DATA];
    char client_id[MAX_DATA];
    
    strcpy(client_id, msg->source);
    
    sscanf(msg->data, "%s %s", command, session_id);
    
    struct session* existing_session = search_session_list(session_id);

    struct lab3message ack;
    struct lab3message* ack_ptr = &ack;
    
    if (existing_session == NULL){ // no such session --> you can create
        
        struct session* created_session = (struct session*)malloc(sizeof(struct session));
        
        created_session->next = NULL;
        strcpy(created_session->session_id, session_id);
        
        
        // add to the session list 
 
        if (sessions == NULL){
            sessions = created_session; // users = list, user = the new struct made in the beginning of the function
        }
        else {
            created_session->next = sessions; // add to the beginning of the array 
            sessions = created_session;
        }
        
        ack_ptr->type = NS_ACK;
//        strcpy(ack_ptr->data, "Successfully created the session!\n");
        
        /*struct logged_in_user* temp = search_user_list(msg->source);
        
        if(temp != NULL){
            strcpy(temp->session_id, session_id);
        }*/
        
//        printf("plss\n");
        // putting the info into the user_list
        struct logged_in_user* put_info = search_user_list(client_id);
        
        if(put_info != NULL){
            strcpy(put_info->session_id, session_id);
        }
        
        int n;
        n = send(fd, ack_ptr, sizeof(struct lab3message), 0);
        

    }

    else {
        ack_ptr->type = NS_NAK;

        strcpy(ack_ptr->data, "Session already created.\n");
        
        send(fd, ack_ptr, sizeof(struct lab3message), 0);
    }
    
}

void broadcast(struct lab3message* msg, int fd){
    
    char client_id[MAX_DATA];
    
    strcpy(client_id, msg->source);
    
    struct logged_in_user* msg_sender = search_user_list(client_id);
    
    
    int n;
    if (msg_sender == NULL){
        printf("The sender isn't logged in yet.\n");
        return;
    }
    
    struct lab3message broadcasting_msg; 
    struct lab3message* broadcasting_msg_ptr = &broadcasting_msg;
    
    broadcasting_msg_ptr->type = MESSAGE;
    strcpy(broadcasting_msg_ptr->source, msg->source);
    strcpy(broadcasting_msg_ptr->data, msg->data);
    broadcasting_msg_ptr->size = sizeof(msg->data);
    
    struct logged_in_user* temp = users;
    while (temp != NULL){
        if ( (strcmp(temp->client_id, client_id) != 0) && (strcmp(temp->session_id, msg_sender->session_id) == 0) ) { // send to everyone in the session except the sender 

            if (temp->fd != fd) {    // not needed, but just checking 
                n = send(temp->fd, broadcasting_msg_ptr, sizeof(struct lab3message), 0);

            }
        }
        temp = temp->next;
    }
}


void list(struct lab3message* msg, int fd){
    // print list of users and the sessions
    
    struct lab3message ack;
    struct lab3message* ack_ptr = &ack;
    
    
    char all_sessions[MAX_DATA];
    char all_users[MAX_DATA];
    
    char whole_list[MAX_DATA];
    
    int user_counter = 0;
    int address_offset = 0;
    int session_counter = 0;
    
    
    
    struct logged_in_user* user_cur = users;
    struct session* session_cur = sessions;

    
    
    
    
    user_counter++;
    session_counter++;
    while(user_cur != NULL){
        address_offset = address_offset + sprintf(address_offset + all_users, "%d. %s JOINED: %s\n", user_counter, user_cur->client_id, user_cur->session_id );
        
        user_cur = user_cur->next;
        user_counter++;
    }
    
    strcat(all_users + address_offset, "\nSessions:\n");
    
    
    address_offset = 0;
    while(session_cur != NULL){
        address_offset = address_offset + sprintf(address_offset + all_sessions, "%d. %s\n", session_counter, session_cur->session_id );
        
        session_cur = session_cur->next;
        session_counter++;
    }
    
    strcat(all_users, all_sessions);
    
    strcpy(ack_ptr->data, all_users);
    ack_ptr->type = QU_ACK;
    ack_ptr->size = sizeof(all_users);
    
    send(fd, ack_ptr, sizeof(struct lab3message), 0);
    
}


struct logged_in_user* search_user_list(char* id){
    
    struct logged_in_user* cur = users;
    
    while (cur != NULL && (strcmp(cur->client_id, id) != 0) ){

        cur = cur->next;
    }
    
    
    return cur;
}

struct session* search_session_list(char* session_id){
    
    struct session* cur = sessions;
    
    while (cur != NULL && (strcmp(cur->session_id, session_id) != 0) ){

        cur = cur->next;
    }
    
    
    return cur;
}

void unicast(struct lab3message* msg, int fd){
    char client_id[MAX_DATA];
    char uni_msg_buffer[MAX_DATA];
    char command[MAX_DATA];
    char to_client_id[MAX_DATA];
    
    sscanf(msg->data, "%s %s", command, to_client_id);
    
    struct lab3message uni_msg;
    struct lab3message* uni_msg_ptr = &uni_msg;
    
    uni_msg_ptr->type = UNIMSG;
    strcpy(uni_msg_ptr->source, msg->source);

    // find 
    struct logged_in_user* temp = search_user_list(to_client_id);
    
    if (temp != NULL){
        
        recv(fd, uni_msg_buffer, sizeof(uni_msg_buffer) , 0);
        
        strcpy(client_id, msg->source);
        strcat(client_id, ": ");
        strcat(client_id, uni_msg_buffer);
       
        strcpy(uni_msg_ptr->data, client_id );
        uni_msg_ptr->size = sizeof(client_id);

        send(temp->fd, uni_msg_ptr, sizeof(struct lab3message), 0);
    }
    else {
        printf("The opponent client does not exit.\n");
    }
    
}
