


#ifndef SERVER_H
#define SERVER_H

#ifdef __cplusplus
extern "C" {
#endif
    
#include "needs.h"
    

//extern struct logged_in_user* users;
//extern struct session* sessions;


void get_message_type(struct lab3message* msg, int fd);
void login_conference(struct lab3message* msg, int fd);
void log_out_conference(struct lab3message* msg, int fd);  
void join_session(struct lab3message* msg, int fd);   
void leave_session(struct lab3message* msg, int fd);    
void new_session(struct lab3message* msg, int fd); 
void broadcast(struct lab3message* msg, int fd);
void list(struct lab3message* msg, int fd);
struct logged_in_user* search_user_list(char* id);
struct session* search_session_list(char* session_id);
void unicast(struct lab3message* msg, int fd);



#ifdef __cplusplus
}
#endif

#endif /* SERVER_H */

