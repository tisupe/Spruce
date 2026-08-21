#include <array>
#include <cstdint>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
using namespace std;

struct DNS_Header {
    uint16_t id = 1234;

    bool qr = 1;
    uint8_t opcode = 0;
    bool aa = 0;
    bool tc = 0;
    bool rd = 0;
    bool ra = 0;
    uint16_t z = 0;
    uint8_t rcode = 0;

    uint16_t qdcount = 0;
    uint16_t ancount = 0;
    uint16_t nscount = 0;
    uint16_t arcount = 0;
};

array<uint8_t, 12> serialize(const DNS_Header &h) {
    uint16_t flags = (h.qr << 15) | (h.opcode << 11) | (h.aa << 10) | (h.tc << 9) | (h.rd << 8) | (h.ra << 7) | (h.z << 4) | (h.rcode << 0);
    array<uint8_t, 12> result;
    result[0] = h.id / 256;
    result[1] = h.id % 256;
    result[2] = flags / 256;
    result[3] = flags % 256;
    result[4] = h.qdcount / 256;
    result[5] = h.qdcount % 256;
    result[6] = h.ancount / 256;
    result[7] = h.ancount % 256;
    result[8] = h.nscount / 256;
    result[9] = h.nscount % 256;
    result[10] = h.arcount / 256;
    result[11] = h.arcount % 256;

    return result;
}

int main() {
    cout << unitbuf;
    cerr << unitbuf;
    setbuf(stdout, NULL);

    struct sockaddr_in clientAddress;
    int udpSocket;

    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        cerr << "Socket creation failed: " << strerror(errno) << "..." << endl;
        return 1;
    }

    int reuse = 1;
    if (setsockopt(udpSocket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse)) < 0) {
        cerr << "SO_REUSEPORT failed: " << strerror(errno) << endl;
        return 1;
    }

    sockaddr_in serv_addr = { .sin_family = AF_INET,
                              .sin_port = htons(2053),
                              .sin_addr = { htonl(INADDR_ANY) },
                            };

    if (bind(udpSocket, reinterpret_cast<struct sockaddr*>(&serv_addr), sizeof(serv_addr)) != 0) {
        cerr << "Bind failed: " << strerror(errno) << endl;
        return 1;
    }

    int bytesRead;
    char buffer[512];
    socklen_t clientAddrLen = sizeof(clientAddress);

    DNS_Header header;
    array<uint8_t, 12> response = serialize(header);


    while (true) {
        // receiving data
        bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);
        if (bytesRead == -1) {
            perror("Error receiving data");
            break;
        }

        buffer[bytesRead] = '\0';
        cout << "Received " << bytesRead << " bytes: " << buffer << endl;

        // sending response
        if (sendto(udpSocket, response.data(), response.size(), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }

    close(udpSocket);

    return 0;
}
