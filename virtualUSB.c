
// THIS COMMAND LINE PROBRAM WILL SEND AN INFO REQUEST TO A KEYSTONE3 SIMULATOR OVER HTTP
// FILL IN THE IP ADDRESS OF THE COMPUTER RUNNING THE SIMULATOR AS THE remoteIPAddr BELOW

// TO IMPLEMENT COMMANDS THAT WOULD TAKE MORE THAN 1 USB PACKET, SEND THE SAME PACKETS WITH SUCCESSIVE send() OPERATIONS
// COMPILE WITH clang virtualUSB.c -o testUSB, OR USE YOU CAN USE gcc DIRECTLY OR ANY OTHER AS YOUR C COMPILER
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
    
    // IF YOU ARE RUNNING THIS PROGRAM ON A DIFFERENT COMPUTER THAN THE ONE RUNNING THE SIMULATOR
    // THE remoteIPAddr SHOULD BE THE PUBLIC IP ADDRESS OF THE COMPUTER RUNNING THE SIMULATOR
    
    // OTHERWISE THE remoteIPAddr SHOULB BE THE LOCAL IP ADDRESS OF THE SAME COMPUTER IF YOU ARE ON DHCP
    // WHICH WILL BE SOMETHING LIKE 192.168.xxx.xxx, OR YOUR PUBLIC IP ADDRRESS IF YOU HAVE A STATIC IP ADDRESS
    
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
    
    char* remoteIPAddr = "192.168.99.254";
  //  char* remoteIPAddr = "111.222.333.444";
    
    if (strcmp((char*)remoteIPAddr, "111.222.333.444") == 0) {
        printf("You must fill in the public address of the computer running the Keystone3 simulator\nThen compile this program again\n");
        return -1;
    }
    else {
        printf("remoteIPAddr = %s\n", (char*)remoteIPAddr);
    }
    
    int thisSocket;
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    
    local_addr.sin_family = AF_INET;
//    inet_pton(AF_INET, (char*)ipAddress, &(local_addr.sin_addr.s_addr));
    inet_pton(AF_INET, (char*)localIPAddr, &(local_addr.sin_addr.s_addr));
//    inet_pton(AF_INET, (char*)remoteIPAddr, &(local_addr.sin_addr.s_addr));
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
    
    struct sockaddr_in sin;  // JUST FOR CHECKING OUT OUTGOING IP ADDRESS AND PORT
    socklen_t len = sizeof(sin);
    
    // RECHECK ASSIGNED LOCAL IP ADDRESS AND PORT
    if (getsockname(thisSocket, (struct sockaddr *)&sin, &len) == -1) {
        printf("getsockname() failed\n");
        close(thisSocket);
        return -1;
    }
    else {
        printf("getsockname() OK\n");
        printf("local ip address = %s\n", inet_ntoa(sin.sin_addr));
        printf("local port = %d\n", ntohs(sin.sin_port));
    }
    
    printf("trying to connect to Keystone3 simulator (timeout about 60 seconds)\n");
    printf("for faster timeout verdict implement non-blocking socket and select()\n");
    
    // IF CONNECTION FAILS WILL TIME OUT IN ABOUT 60 SECONDS
    // FOR FASTER VERDICT IMPLEMENT NON-BLOCKING SOCKET AND select(), THEN BLOCK SOCKET AGAIN AFTER RESPONSE
    
    if (connect(thisSocket, (struct sockaddr *)(&keystone3_addr), sizeof(keystone3_addr)) < 0 ) {
        close(thisSocket);
        printf("could not connect to Keystone3 simulator\n\n");
        return -1;
    }
    else {
        printf("connected to Keystone3 simulator\n");

    }
    
    char responseData[65536];
    char packetBuffer[64];
    
    printf("sending Info Request packet by HTTP\n");
    
    send(thisSocket, (const char*)keystonePacket, keystonePacketSize, 0);
    
MORE_FETCH_DATA:
    
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
        goto MORE_FETCH_DATA;
    }
    
    responseData[totalDataLength] = 0x00;  // TERMINATE TO BE SAFE
    
    printf("responseData = %s\n", (char*)responseData);
    
    close(thisSocket);
    
    return 0;
}
