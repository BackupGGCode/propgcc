/* xbee-test.c - Xbee Wi-Fi test program */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>

#define IPADDR  "10.0.1.66"
#define PORT    0x2616          // supposedly the default for the Xbee Wi-Fi

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

int ConnectSocket(char *hostName, short port);
void DisconnectSocket(int socket);
int SocketDataAvailableP(int socket, int timeout);
int SendSocketData(int socket, char *buf, int len);
int ReceiveSocketData(int socket, char *buf, int len);

/* main - the main function */
int main(int argc, char *argv[])
{
    char buf[1024];
    int xbee, cnt;
    
    if ((xbee = ConnectSocket(IPADDR, PORT)) < 0) {
        printf("Failed to connect to %s:%d\n", IPADDR, PORT);
        return 1;
    }
    printf("Connected\n");
    
    if (SendSocketData(xbee, "Hello!", 6) < 0) {
        printf("Failed to send data to %s:%d\n", IPADDR, PORT);
        return 1;
    }
    printf("Data sent\n");
    
    if ((cnt = ReceiveSocketData(xbee, buf, sizeof(buf))) < 0) {
        printf("Failed to receive data from %s:%d\n", IPADDR, PORT);
        return 1;
    }
    buf[cnt] = '\0';
    printf("Data received: %s\n", buf);
    
    sleep(10);

    DisconnectSocket(xbee);
    printf("Disconnected\n");
    
    return 0;
}

/* ConnectSocket - connect to the server */
int ConnectSocket(char *hostName, short port)
{
    int enable = TRUE;
    struct hostent *hostEntry;
    struct sockaddr_in addr;
    struct linger lstruct;
    int sock;

    /* create the socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        return -1;

#if 0
    /* turn on the "linger" flag */
    lstruct.l_onoff = 1;
    lstruct.l_linger = 10;
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *)&lstruct, sizeof(lstruct));

    /* turn off output buffering */
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(enable));
#endif

    /* setup the address */
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    /* get the host address by address in dot notation */
    if (isdigit(hostName[0])) {
        unsigned long hostAddr = inet_addr(hostName);
        addr.sin_addr = *(struct in_addr *)&hostAddr;
    }
    
    /* get the host address by name */
    else {
        if ((hostEntry = gethostbyname(hostName)) == NULL) {
            close(sock);
            return -1;
        }
        addr.sin_addr = *(struct in_addr *)*hostEntry->h_addr_list;
    }
    
    /* connect to the server */
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        close(sock);
        return -1;
    }

    /* return the socket */
    return sock;
}

/* DisconnectSocket - close a connection */
void DisconnectSocket(int sock)
{
    struct timeval tv;
    fd_set sockets;
    char buf[512];

    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    /* wait for the close to complete */
    for (;;) {
        FD_ZERO(&sockets);
        FD_SET(sock, &sockets);
        if (select(sock + 1, &sockets, NULL, NULL, &tv) > 0) {
            if (FD_ISSET(sock, &sockets))
                if (recv(sock, buf, sizeof(buf), 0) == 0)
                    break;;
        }
        else
            break;
    }

    /* shutdown the socket */
    shutdown(sock, SHUT_RDWR);

    /* close the socket */
    close(sock);
}

/* SocketDataAvailableP - check for data being available on a socket */
int SocketDataAvailableP(int sock, int timeout)
{
    struct timeval timeVal;
    fd_set sockets;
    int cnt;

    /* setup the read socket set */
    FD_ZERO(&sockets);
    FD_SET(sock, &sockets);

    timeVal.tv_sec = timeout / 1000;
    timeVal.tv_usec = (timeout % 1000) * 1000;

    /* check for data available */
    cnt = select(sock + 1, &sockets, NULL, NULL, timeout < 0 ? NULL : &timeVal);

    /* return whether data is available */
    return cnt > 0 && FD_ISSET(sock, &sockets);
}

/* SendSocketData - send socket data */
int SendSocketData(int sock, char *buf, int len)
{
    return send(sock, buf, len, 0);
}

/* ReceiveSocketData - receive socket data */
int ReceiveSocketData(int sock, char *buf, int len)
{
    return recv(sock, buf, len, 0);
}
