
#ifndef NEEDS_H
#define NEEDS_H

#ifdef __cplusplus
extern "C" {
#endif


#define MAX_NAME 1000
#define MAX_DATA 1000
#define MAX_CLIENT 150

// message types 
#define LOGIN 0
#define LO_ACK 1 
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NAK 6 
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define MESSAGE 10
#define QUERY 11 
#define QU_ACK 12
#define NS_NAK 13
#define UNIMSG 14

//static struct logged_in_user* users;
//static struct session* sessions;
//extern char server_port_number[MAX_DATA];
    

//static session* sessions_list;
//struct global {
//    static struct logged_in_user* users;
//    static struct session* sessions;
//};

    
    
struct lab3message {
    unsigned int type;
    unsigned int size;
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
};

extern int fdmax; 

struct logged_in_user {
    
    
    char client_id[MAX_DATA];
    char password[MAX_DATA];
    
//    unsigned char source[MAX_NAME];
    int fd;
    
    //int joined_session; // 0 for not joined a session yet and 1 for joined 
    char session_id[MAX_DATA]; // the name of the session 
    
    struct logged_in_user* next;
    
};


struct session {
    char session_id[MAX_DATA];
    
//    char* array_client[50]; // 50 people only per session
    
    struct session* next;
};



#ifdef __cplusplus
}
#endif

#endif /* NEEDS_H */

