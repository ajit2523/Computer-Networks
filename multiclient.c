#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
 
int main(int argc, char *argv[])
{
    // IP Address where server code is running, port number for server, mode of running
    if (!strcmp(argv[3], "s"))
    {
        // code for server part
        int socketid_s, s;
        char recvBuf[1024] = {0};
        struct sockaddr_in addrport_s;
        int addrLen = sizeof(addrport_s);
 
        int port_s = strtoul(argv[2], NULL, 10);
        socketid_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 
        addrport_s.sin_family = AF_INET;
        addrport_s.sin_port = htons(port_s);
        inet_pton(AF_INET, argv[1], &addrport_s.sin_addr);
 
        if (bind(socketid_s, (struct sockaddr *)&addrport_s, sizeof(addrport_s)) < 0) // binded IP address and port number to the socket that we created.
        {
            perror("Bind failure.");
            exit(EXIT_FAILURE);
        }
 
        if (listen(socketid_s, 1) < 0) // only 1 client is to be handled.
        {
            perror("Listen failure.");
            exit(EXIT_FAILURE);
        }
        else
        {
            printf("Listening for connections...\n");
        }
 
        while (1)
        {
            s = accept(socketid_s, (struct sockaddr *)&addrport_s, (socklen_t *)&addrLen);
 
            if (s < 0)
            {
                perror("Accept failure.");
                exit(EXIT_FAILURE);
            }
 
            while (1)
            {
                int count_s = recv(s, recvBuf, 1024, 0); // will have the length of the message.
                if (!strcmp(recvBuf, "EXIT\n"))
                {
                    printf("Closing.\n");
                    close(s);
                    close(socketid_s);
                    exit(1);
                }
                else if (!strcmp(recvBuf, "LIST\n"))
                {
                    printf("Directory: \n");
                    DIR *d;
                    struct dirent *dir;
                    d = opendir(".");
                    if (d)
                    {
                        strcpy(recvBuf, "");
                        while ((dir = readdir(d)) != NULL)
                        {
                            // printf("%s\n",dir->d_name);
                            strcat(recvBuf, dir->d_name);
                            strcat(recvBuf, "\n");
                        }
                        closedir(d);
                    }
                    send(s, recvBuf, sizeof(recvBuf), 0);
                    strcpy(recvBuf, "");
                }
                else
                {
                    printf("Message received by server: %s\n", recvBuf);
                    char *cmd = strtok(recvBuf, " ");
                    if (!strcmp(cmd, "GET"))
                    {
                        char *fileName = strtok(NULL, " ");
                        if (fileName != NULL)
                        {
                            char *numberOfBytes = strtok(NULL, " ");
                            if (numberOfBytes != NULL)
                            {
                                int n = atoi(numberOfBytes);
                                char readBuffer[1024] = {0};
                                int fd = open(fileName, O_RDONLY, S_IRUSR | S_IWUSR);
                                int readCount = read(fd, readBuffer, n);
                                if (readCount == -1)
                                {
                                    printf("Error reading from file.\n");
                                    strcpy(recvBuf, "Error");
                                }
                                else
                                {
                                    strcpy(recvBuf, readBuffer);
                                }
                                close(fd);
                            }
                            else
                            {
                                printf("No of bytes not provided.\n");
                                strcpy(recvBuf, "Error");
                            }
                        }
                        else
                        {
                            printf("No file name provided.\n");
                            strcpy(recvBuf, "Error");
                        }
                    }
                    else
                    {
                        printf("Invalid Command\n");
                        strcpy(recvBuf, "Error");
                    }
                    send(s, recvBuf, sizeof(recvBuf), 0);
                    strcpy(recvBuf, "");
                }
            }
        }
 
        printf("Closing.\n");
        close(s);
        close(socketid_s);
    }
 
    else if (!strcmp(argv[3], "c"))
    {
        // code for client part
        int socketid_c, c;
 
        char sendBuf[1024] = {0}, recBuf[1024] = {0};
        struct sockaddr_in addrport_c, addrport_s;
        int addrLen = sizeof(addrport_c);
 
        int port_s = strtoul(argv[2], NULL, 10);
        socketid_c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
 
        addrport_s.sin_family = AF_INET;
        addrport_s.sin_port = htons(port_s);
        inet_pton(AF_INET, argv[1], &addrport_s.sin_addr);
 
        c = connect(socketid_c, (struct sockaddr *)&addrport_s, sizeof(addrport_s));
 
        if (c < 0)
        {
            printf("Could not find the server on %s:%u.\n", argv[1], port_s);
            exit(EXIT_FAILURE);
        }
 
        if (getsockname(socketid_c, (struct sockaddr *)&addrport_c, (socklen_t *)&addrLen) == -1)
        {
            perror("Error getting socket name.");
            exit(EXIT_FAILURE);
        };
 
        char *c_ip = inet_ntoa(addrport_c.sin_addr);
        unsigned short c_port = ntohs(addrport_c.sin_port);
        printf("%s:%u is connected to %s:%u\n", c_ip, c_port, argv[1], port_s);
 
        printf("Enter Message: ");
        fgets(sendBuf, 1024, stdin);
 
        while (1)
        {
            send(socketid_c, sendBuf, sizeof(sendBuf), 0);
            if (!strcmp(sendBuf, "EXIT\n"))
            {
                break;
            }
            else if (!strcmp(sendBuf, "LIST\n"))
            {
                recv(socketid_c, recBuf, 1024, 0);
                printf("Files in server dir:\n %s\n\n", recBuf);
                strcpy(sendBuf, "");
                strcpy(recBuf, "");
                printf("Enter command, filename and number of bytes: ");
                fgets(sendBuf, 1024, stdin);
            }
            else
            {
 
                recv(socketid_c, recBuf, 1024, 0);
                printf("Message received by client: %s\n\n", recBuf);
                char *cmd = strtok(sendBuf, " ");
                char *fileName = strtok(NULL, " ");
                char *numberOfBytes = strtok(NULL, " ");
                if (strcmp(recBuf, "Error"))
                {
                    int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                    if (fd == -1)
                        printf("Error opening file.\n");
                    int bytes = write(fd, recBuf, strlen(recBuf));
                    if (bytes == -1)
                        printf("Error writing to file.\n");
                    close(fd);
                }
                else
                {
                    printf("Error. Try again.\n");
                }
                strcpy(sendBuf, "");
                strcpy(recBuf, "");
                printf("Enter command, filename and number of bytes: ");
                fgets(sendBuf, 1024, stdin);
            }
        }
 
        close(socketid_c);
        close(c);
    }
 
    else
    {
        printf("Invalid argument.\n");
    }
    return 0;
}
