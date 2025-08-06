© 2025 The Pen

Provided the above copyright notice is also preserved, permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


// THIS COMMAND LINE PROBRAM WILL SEND A SAMPLE INFO REQUEST TO A KEYSTONE3 SIMULATOR OVER HTTP
// FILL IN THE IP ADDRESS OF THE COMPUTER RUNNING THE SIMULATOR AS THE remoteIPAddr BELOW

// FOR COMMANDS THAT TAKE MORE THAN 1 USB PACKET, SEND THE PACKETS AS A BATCH WITH SUCCESSIVE send() OPERATIONS
// COMPILE WITH clang virtualUSB2.c -o testUSB, OR USE YOU CAN USE gcc DIRECTLY OR ANY OTHER C COMPILER
// RUN WITH ./testUSB ON MAC/UNIX FROM A COMMAND LINE ONCE THE SIMULATOR IS RUNNING, OR JUST testUSB ON WINDOWS


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <ifaddrs.h>

int main() {
    
// #ifdef __cplusplus
//    printf("Compiling as C++ v=%d\n", __cpluplus);
// #else
//    printf("Compiling as C\n");
// #endif
    

    // THE localIPAddr WILL BE AUTOMATICALY SET BY THIS PROGRAM
    
    // GET OUR LOCAL IP ADDRESS
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];
    memset(host, 0, NI_MAXHOST);
    
    if (getifaddrs(&ifaddr) == -1) {
        printf("getifaddrs FAILED\n");
        perror("getifaddrs");
        return -1;
    } else {
        printf("getifaddrs() OK\n");
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }
        
        family = ifa->ifa_addr->sa_family;
        
        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return -1;
            }
            
            if (strcmp(ifa->ifa_name, "lo0") != 0) { // Exclude loopback interface
                //   ipAddress = host;
                break;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    char *localIPAddr = (char *) host;
    printf("localIPAddr = %s\n", localIPAddr);
    
    // IF YOU ARE RUNNING THIS PROGRAM ON A DIFFERENT COMPUTER THAN THE ONE RUNNING THE SIMULATOR
    // THE remoteIPAddr SHOULD BE THE PUBLIC IP ADDRESS OF THE COMPUTER RUNNING THE SIMULATOR
    
    // OTHERWISE THE remoteIPAddr SHOULB BE THE LOCAL IP ADDRESS OF THE SAME COMPUTER IF YOU ARE ON DHCP
    // WHICH WILL BE SOMETHING LIKE 192.168.xxx.xxx, OR YOUR PUBLIC IP ADDRESS IF YOU HAVE A STATIC IP ADDRESS

    // REMOTE IP ADDRESS
    char* remoteIPAddr = "xxx.xxx.xxx.xxx";
    
    if (strcmp((char*)remoteIPAddr, "xxx.xxx.xxx.xxx") == 0) {
        printf("You must fill in the remote ip address of the computer running the Keystone3 Simulator\nThen compile this program again\n");
        return -1;
    }
    else {
        printf("remoteIPAddr = %s\n", (char*)remoteIPAddr);
    }
    
    int thisSocket;
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    
    local_addr.sin_family = AF_INET;
    inet_pton(AF_INET, (char*)localIPAddr, &(local_addr.sin_addr.s_addr));
    local_addr.sin_port = htons(0);  // ANY PORT
    
    thisSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (thisSocket < 0) {
        printf("could not make socket\n");
        return -1;\
    }
    else {
        printf("made socket on %s\n", (char*)localIPAddr);
    }
    
    int enable = 1;
    // NOT CRITICAL FOR OPERATION
    if (setsockopt(thisSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(thisSocket);
        return -1;
    }
    else {
        printf("setsockopt() SO_REUSEADDR\n");
    }
    
    if (bind(thisSocket, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        printf("could not bind socket\n");
        close(thisSocket);
        return -1;
    }
    else {
        printf("bound socket\n");
    }
    
    struct sockaddr_in keystone3_addr;
    keystone3_addr.sin_family = AF_INET;
    
    char keystonePacket[64];
    
    // TEST OF INFO REQUEST (INS = 5)
    keystonePacket[0] = 0x00;  // EAPDUY CLA
    
    keystonePacket[1] = 0x00;  // INFO REQUEST
    keystonePacket[2] = 0x05;
    
    keystonePacket[3] = 0x00;  // ONE PACKET
    keystonePacket[4] = 0x01;
    
    keystonePacket[5] = 0x00;  // FIRST PACKET INDEX
    keystonePacket[6] = 0x00;
    
    keystonePacket[7] = 0x01;  // REQUEST ID
    keystonePacket[8] = 0x23;
    
    // NO DATA OUTGOING DATA FILES FOER THIS ONE
    
    uint32_t recvBytes = 0;
    uint32_t totalDataLength = 0;
    uint32_t keystonePacketSize = 9;  // ALL THAT'S REQUIRED FOR THIS ONE
    
    inet_pton(AF_INET, (char*)remoteIPAddr, &(keystone3_addr.sin_addr.s_addr));
    keystone3_addr.sin_port = htons(81);  // COULD BE ANYTHING REALLY THAT MATCHES THE SIMULATOR SETTING, BUT BEST <1024
    
    struct sockaddr_in sin;  // JUST FOR CHECKING OUR OUTGOING IP ADDRESS AND PORT
    socklen_t len = sizeof(sin);
    
    // RECHECK ASSIGNED IP ADDRESS AND PORT
    if (getsockname(thisSocket, (struct sockaddr *)&sin, &len) == -1) {
        printf("getsockname() failed\n");
        close(thisSocket);
        return -1;
    }
    else {
        printf("getsockname() recheck OK\n");
        printf("localIPAddr = %s\n", inet_ntoa(sin.sin_addr));
        printf("localPort = %d\n", ntohs(sin.sin_port));
    }
    
#include <fcntl.h> // Required for fcntl and O_NONBLOCK
#include <errno.h>  // errno
#include <time.h>
    
    long arg = fcntl(thisSocket, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(thisSocket, F_SETFL, arg);
    
    fd_set read_fds;
    fd_set write_fds;
    
    FD_ZERO(&write_fds);
    FD_ZERO(&write_fds);
    
    int resFetch;
    int val_read1;
    int cResult;
    int sentBytes;
    
    char responseData[65536];
    char packetBuffer[64];
    
    // START NEW CODE
    
// case EINTR: errString = "interrupted";                 //  4
// case EAGAIN: errString = "empty rx queue";             // 35
// case EINPROGRESS: errString = "in progress";           // 36
// case ETIMEDOUT: errString = "recv timeout";            // 60
// case ENOTCONN: errString = "not connected";            // 57
// THIS IS SAME AS EAGAIN HERE, MIGHT BE DIFFERENT IN WIN
// case EWOULDBLOCK: errString = "would block";           // 35
// case EHOSTUNREACH: errString = "host unreachable";     // 65
// case ECONNREFUSED: errString = "connection refused";   // 61
// case ECONNRESET: errString = "connection reset";       // 54
// case ENOTSOCK: errString = "operation on non-socket";  // 38
// case EBADF: errString = "bad file descriptor";         //  9
    
    if ((cResult = connect(thisSocket, (struct sockaddr *)&keystone3_addr, sizeof(keystone3_addr))) < 0) {
    
        if ((errno == EINPROGRESS) ||
            (errno == EWOULDBLOCK)) {
            
            if (errno == EINPROGRESS) {
      //        printf("errno=EINPROGRESS setting write_fds=%d for %s\n", thisSocket, (char*)remoteIPAddr);
            }
            else if (errno == EWOULDBLOCK) {
      //        printf("errno=EWOULDBLOCK setting write_fds=%d for %s\n", thisSocket, (char*)remoteIPAddr);
            }
            
            FD_SET(thisSocket, &write_fds);  // WAITING TO WRITE
        }
        
        else {
            if ((errno == 61) || (errno == 65)) {
                if (errno == 61) {
                    printf("connection refused IP=%s\n", (char*)remoteIPAddr);
                }
                else if (errno == 65) {
                    printf("host unreachable IP=%s\n", (char*)remoteIPAddr);
                }
                else {
                    printf("connect errno=%d IP=%s\n", errno, (char*)remoteIPAddr);
                }
            }

            close(thisSocket);
            return -1;
        }
    }  // IF NOT CONNECTED YET
    
    else {
 
        int sentBytes = send(thisSocket, (const char*)keystonePacket, keystonePacketSize, 0);
        printf("sent packet by HTTP to %s sentBytes = %d \n", (char*)remoteIPAddr, sentBytes);
        FD_SET(thisSocket, &read_fds);  // WAITING TO READ
        
    }  // IF READY TO SEND IMMEDIATELY
        
    struct timeval tv;

    tv.tv_sec = 2;  // DOES SELECT CLEAR THIS TOO?
    tv.tv_usec = 0;

  SINGLE_FETCH_SELECT:
    
  // WAITING FOR SOMEONE TO BE READY TO EITHER READ OR WRITE
    resFetch = select(thisSocket + 1, &read_fds, &write_fds, NULL, &tv);
    
    if (resFetch < 0) {
        printf("error return from select() < 0\n");
        close(thisSocket);
        return -1;
    }
    
    else if (resFetch == 0) {
        printf("error return from select() = 0\n");
        close(thisSocket);
        return -1;
    }
    
    else if (FD_ISSET(thisSocket, &write_fds)) {
        
        // MSG_NOSIGNAL HERE PREVENTS A SILENT CRASH VIA SIGPIPE IF send() FAILED DUE TO BAD SOCKET, ETC.
        sentBytes = send(thisSocket, (const char*)keystonePacket, keystonePacketSize, MSG_NOSIGNAL);
        
        if (sentBytes < 0){
            printf("could not send packet to Simulator\n");
            close(thisSocket);
            return -1;
        }
        else {
            printf("sent packet by HTTP to %s sentBytes = %d \n", (char*)remoteIPAddr, sentBytes);
        }
        
        FD_ZERO(&write_fds);
        FD_ZERO(&read_fds);
        FD_SET(thisSocket, &read_fds);  // WAITING TO READ
        
        goto SINGLE_FETCH_SELECT;  // GOING BACK FOR THE RECV
    }
    
    else if (FD_ISSET(thisSocket, &read_fds)) {
        recvBytes = recv(thisSocket, (char*)packetBuffer, 64, 0);
        
        uint16_t numberOfPackets = ((uint16_t)packetBuffer[3] >> 8) + packetBuffer[4];
        printf("numberOfPackets = %d\n", numberOfPackets);
        
        uint16_t thisPacketIndex = ((uint16_t)packetBuffer[5] >> 8) + packetBuffer[6];
        printf("thisPacketIndex = %d\n", thisPacketIndex);
        
        uint16_t responseStatus = ((uint16_t)packetBuffer[recvBytes - 2] >> 8) + packetBuffer[recvBytes - 1];
        
        if (responseStatus == 0) {
            printf("responseStatus = %d OK\n", responseStatus);
        }
        else {
            printf("responseStatus = %d error\n", responseStatus);
        }
        
        if (recvBytes > 11) {
            memcpy((char*)responseData + totalDataLength, (char*)packetBuffer + 9, recvBytes - 11);
            totalDataLength += recvBytes - 11;
        }
        
        if ((numberOfPackets - thisPacketIndex) > 1) {
            FD_ZERO(&write_fds);   // SHOULDN'T WE ALWAYS DO THIS?
            FD_ZERO(&read_fds);
            FD_SET(thisSocket, &read_fds);
            goto SINGLE_FETCH_SELECT;
        }
    }
    
    responseData[totalDataLength] = 0x00;  // TERMINATE TO BE SAFE
    
    printf("responseData = %s\n", (char*)responseData);
    
    close(thisSocket);
    
    return 0;
}
