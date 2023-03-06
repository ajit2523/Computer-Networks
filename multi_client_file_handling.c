#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

    
#define BUFSIZE 1024

int client_ids[1000];
int num_clients;
int num;
int talk_client1;
int talk_client2;
int next_client;

sem_t mutex;

void * client_handler(void *param){
    int num_clients = *(int *)param;
    char buffer[BUFSIZE];
    
        memset(buffer, 0, BUFSIZE);
        int n = read(client_ids[num_clients-1], buffer, BUFSIZE - 1);
        if (n < 0)
            printf("ERROR reading from socket");
        if ((strncmp(buffer, "HELO", 4)) == 0)
        {
            printf("Received message: %s\n", buffer);
            char *msg = "MESG What is your name?\n";
            n = write(client_ids[num_clients-1], msg, strlen(msg));
            if (n < 0)
                printf("ERROR writing to socket");
        }
        else
        {
            printf("Invalid command\n");
        }
        memset(buffer, 0, BUFSIZE);
        char username[BUFSIZE];
        memset(username, 0, BUFSIZE);
        n = read(client_ids[num_clients-1], username, BUFSIZE - 1);
        printf("Received message: %s \n", username);
        if (n < 0)
            printf("ERROR reading from socket");
        if ((strncmp(username, "MESG ", 4)) ==  0)
        {
            memcpy(username, username + 5, strlen(username));
            username[strlen(username) - 1] = '\0';
            
            struct sockaddr_in client;
            bzero(&client, sizeof(client));
            socklen_t len = sizeof(client);
            char ip[16];
            int name = getsockname(client_ids[num_clients-1], (struct sockaddr *)&client, &len);
            if (name == -1)
            {
                printf("SockName failed");
                exit(1);
            }
            inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
            strcat(username, "@");
            strcat(username, ip);
            strcat(username, " ");
            sem_wait(&mutex);
            int fd = open("user.txt", O_WRONLY | O_APPEND);
            if (fd < 0)
            {
                printf("Error in opening file");
                exit(1);
            }
            write(fd, username, strlen(username));
            close(fd);
            sem_post(&mutex);
        }
        while(1){
        memset(buffer, 0, BUFSIZE);
        n = read(client_ids[num_clients-1], buffer, BUFSIZE - 1);
        if (n < 0)
            printf("ERROR reading from socket");
        
        if ((strncmp(buffer, "EXIT ", 4)) == 0){
                
                sem_wait(&mutex);
                int fd = open("user.txt", O_RDWR | O_APPEND | O_CREAT, 0666);
                if (fd < 0)
                {
                    printf("Error in opening file");
                    exit(1);
                }
                
                char *content = (char *)malloc(1000 * sizeof(char));
                int n = read(fd, content, 1000);
                if (n < 0)
                {
                    perror("Error in reading file");
                    exit(1);
                }
                
                char *token = strtok(content, " ");
                char *new_content = (char *)malloc(1000 * sizeof(char));
                while (token != NULL)
                {
                    if (strncmp(token, username , strlen(username)-1) != 0)
                    {
                        strcat(new_content, token);
                        strcat(new_content, " ");
                    }
                    token = strtok(NULL, " ");
                }
                ftruncate(fd, 0);
                
                write(fd, new_content, strlen(new_content));
                close(fd);
                sem_post(&mutex);
                close(client_ids[num_clients-1]);
                printf("Client%d disconnected\n", num_clients);
                break;
        }
        else if(strncmp(buffer,"LIST ",4) == 0){
            sem_wait(&mutex);
            int fd = open("user.txt", O_RDWR | O_APPEND | O_CREAT, 0666);
            if (fd < 0)
            {
                printf("Error in opening file");
                exit(1);
            }
            char *content = (char *)malloc(1000 * sizeof(char));
            int n = read(fd, content, 1000);
            if (n < 0)
            {
                printf("Error in reading file");
                exit(1);
            }
            printf("Content: %s\n", content);
            char *new_content = (char *)malloc(1000 * sizeof(char));
            strcpy(new_content, "CURR ");
            strcat(new_content, content);

            n = write(client_ids[num_clients-1], new_content, strlen(new_content));
            if (n < 0)
                printf("ERROR writing to socket");
            close(fd);
            sem_post(&mutex);
        }
        else if(strncmp(buffer,"MESG ",4)==0){
            memcpy(buffer, buffer + 5, strlen(buffer));
            int flag =0;
            for(int i =0;i<strlen(buffer);i++){
                if(buffer[i]=='@'){
                    flag =1;
                    break;
                }
            }
            if(flag==1){
                printf("%s wants to connect with %s\n",username,buffer);

                sem_wait(&mutex);
                int fd = open("user.txt", O_RDWR | O_APPEND | O_CREAT, 0666);
                if (fd < 0)
                {
                    printf("Error in opening file");
                    exit(1);
                }
                char *content = (char *)malloc(1000 * sizeof(char));
                int n = read(fd, content, 1000);
                if (n < 0)
                {
                    printf("Error in reading file");
                    exit(1);
                }
                char *token = strtok(content, " ");
                int check=0;
                while(token!=NULL){

                    if(strncmp(token,buffer,strlen(buffer)-1)==0){
                        printf("User found\n");
                        char *new_content = "201 available";
                        n = write(client_ids[num_clients-1], new_content, strlen(new_content));
                        if (n < 0){
                            printf("ERROR writing to socket");
                        }
                        else{
                            printf("Message sent\n");
                        }
                        
                        check=1;
                        break;
                    }
                    token = strtok(NULL, " ");
                }
                if(check==0){
                    char *new_content = "404 not found";
                    n = write(client_ids[num_clients-1], new_content, strlen(new_content));
                    if (n < 0)
                        printf("ERROR writing to socket");
                }
                close(fd);
                sem_post(&mutex);

            }

        }
        else
        {
            printf("Invalid command\n");
        }
        }
    pthread_exit(NULL);
}


void *accept_client(void * param){
    sem_init(&mutex, 0, 1);
    int sock = *(int *)param;
    num_clients=0;
    while(1){
        if(talk_client2==7){
            break;
        }
        struct sockaddr_in client1;
        socklen_t client_size = sizeof(client1);
        client_ids[num_clients]= accept(sock, (struct sockaddr *)&client1, &client_size);
        if (client_ids[num_clients] < 0)
        {
            printf("Error in accepting client");
            exit(1);
        }
        num_clients++;
        if(num_clients==1){
            talk_client1 = num_clients;
        }
        else if(num_clients==2){
            talk_client2 = num_clients;
        }
        else if(num_clients==3){
            next_client = num_clients;
        }
        printf("Client%d connected\n", num_clients);
        
        pthread_t acc;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&acc, &attr, client_handler, (void *)&num_clients);

    }
    exit(0);

}
    
int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Invalid arguments");
        exit(1);
    }
    if (argv[3][0] == 'c')
    {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == -1)
        {
            printf("Socket creation failed");
            exit(1);
        }
    
        printf("Socket created\n");
    
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(atoi(argv[2]));
        server.sin_addr.s_addr = inet_addr(argv[1]);
    
        int conn = connect(sock, (struct sockaddr *)&server, sizeof(server));
        if (conn == -1)
        {
            printf("Connection failed");
            exit(1);
        }
        struct sockaddr_in client;
        bzero(&client, sizeof(client));
        socklen_t len = sizeof(client);
    
        char client_IP[16];
        int name = getsockname(sock, (struct sockaddr *)&client, &len);
        if (name == -1)
        {
            printf("SockName failed");
            exit(1);
        }
        inet_ntop(AF_INET, &client.sin_addr, client_IP, sizeof(client_IP));
        unsigned int client_PORT = ntohs(client.sin_port);
    
        printf("ip address of client - %s & port number - %u is connected to ip address of server %s & port number - %u\n", client_IP, client_PORT, inet_ntoa(server.sin_addr), ntohs(server.sin_port));
        
        char buffer[BUFSIZE];
        // memset(buffer, 0, BUFSIZE);
        // printf("ENTER USERNAME :");
        // fgets(buffer, sizeof(buffer), stdin);
        char msg[BUFSIZE];
        memset(msg, 0, BUFSIZE);
        char * helo = "HELO";

        int n = write(sock, helo, strlen(helo));
    
        if(n<0){
            printf("Writing failed");
            exit(1);
        }
        else{
            printf("HELO command sent to server\n");
        }
        memset(buffer, 0, BUFSIZE);
        n = read(sock, buffer, BUFSIZE - 1);
        if(n<0){
            printf("Reading failed");
            exit(1);
        }
        if(strcmp(buffer, "MESG What is your name?\n") != 0){
            printf("Error in HELO command");
        }
        printf("What is your name?");
        memset(buffer, 0, BUFSIZE);
        fgets(buffer, sizeof(buffer), stdin);
        char username[BUFSIZE];
        strcpy(username, buffer);
        

        sprintf(msg, "MESG %s",buffer);
        n = write(sock, msg, strlen(msg));
        if(n<0){
            printf("Writing failed");
            exit(1);
        }
        else{
            printf("Username sent to server\n");
        }
        while(1){
        
            memset(buffer, 0, BUFSIZE);
            printf("\nEnter message: ");
            fgets(buffer, BUFSIZE, stdin);
            int flag=0;
            for(int i = 0;i<strlen(buffer);i++){
                if(buffer[i] == '@'){
                    flag=1;
                    break;
                }
            }
            if(flag){

                memset(msg, 0, BUFSIZE);
                sprintf(msg, "MESG %s",buffer);
                n = write(sock, msg, strlen(msg));
                if(n<0){
                    printf("Writing failed");
                    exit(1);
                }
                else{
                    printf("Client name to communicate with sent to the server\n");
                    while (flag)
                    {   
                        printf("Waiting for server response\n");
                        memset(buffer, 0, BUFSIZE);
                        n = read(sock, buffer, BUFSIZE - 1);
                        
                        if (n < 0)
                            printf("ERROR reading from socket");
                        if(strcmp(buffer,"404 not found") == 0){
                            printf("Client not found\n");
                            flag=0;
                            break;
                        }
                        else{
                            printf("%s\n", buffer);
                        }
                    }
                }
            }
            
            
            if((strncmp(buffer,"EXIT ",4)) == 0){
                n = write(sock, buffer, strlen(buffer));
            if (n < 0){
                printf("ERROR writing to socket");
            }
                printf("Client chose exit command\n");
                close(sock);
                return 0;
            }
            if(strncmp(buffer,"LIST ",4) == 0){
                n = write(sock, buffer, strlen(buffer));
            if (n < 0){
                printf("ERROR writing to socket");
            }
                memset(buffer, 0, BUFSIZE);
                n = read(sock, buffer, BUFSIZE - 1);
                memcpy(buffer, buffer + 5, strlen(buffer));
                if (n < 0)
                    printf("ERROR reading from socket");
                
                printf("List of all clients connected to server\n");
                char *token = strtok(buffer, " ");
                while (token != NULL)
                {
                    
                    if(strncmp(token, username ,strlen(username)-1) != 0){
                        printf("%s\n", token);
                    }
                    token = strtok(NULL, " ");
                }    
            }
    
        }
        close(sock);
    }
    else if (argv[3][0] == 's')
    {
        int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == -1)
        {
            printf("Socket creation failed");
            exit(1);
        }
        printf("Socket created\n");
    
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR , &(int){1}, sizeof(int));
    
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(atoi(argv[2]));
        server.sin_addr.s_addr = inet_addr(argv[1]);
    
        int binded = bind(sock, (struct sockaddr *)&server, sizeof(server));
        if (binded == -1)
        {
            printf("server Binding failed");
            exit(1);
        }
        printf("Binded\n");
    
        int listenfd = listen(sock, 100);
        if (listenfd == -1)
        {
            printf("Listen failed");
            exit(1);
        }
        printf("Listening\n");

        pthread_t acc;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&acc, &attr, accept_client, (void *)&sock);

        pthread_join(acc, NULL);
        close(sock);
    }
    
    return 0;
}
