#include <array>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <vector>
#include "handler.hpp"
using namespace std;

int main() {
    cout << unitbuf;
    cerr << unitbuf;
    setbuf(stdout, NULL);

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
    struct sockaddr_in clientAddress;
    socklen_t clientAddrLen = sizeof(clientAddress);

    dnsHeader header;
    array<uint8_t, 4> ip = {8,8,8,8};
    array<uint8_t, 12> headerBytes = serialize(header);
    vector<uint8_t> questionBytes = encodeQuestion("codecrafters.io", 1, 1);
    vector<uint8_t> answerBytes = encodeAnswer("codecrafters.io", 1, 1, 60, ip);

    vector<uint8_t> response;
    response.insert(response.end(),headerBytes.begin(),headerBytes.end());
    response.insert(response.end(),questionBytes.begin(),questionBytes.end());
    response.insert(response.end(),answerBytes.begin(),answerBytes.end());

    while (true) {
        // receiving
        bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);
        if (bytesRead == -1) {
            perror("Error receiving data");
            break;
        }

        buffer[bytesRead] = '\0';
        cout << "Received " << bytesRead << " bytes: " << buffer << endl;

        for (auto b : response) printf("%02x ", b);
        printf("\n");

        // sending
        if (sendto(udpSocket, response.data(), response.size(), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }
    close(udpSocket);

    return 0;
}
