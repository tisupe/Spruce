#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include "handler.hpp"
#include <unordered_map>

using namespace std;

int main(int argc, char *argv[]) {
    cout << unitbuf;
    cerr << unitbuf;
    setbuf(stdout, NULL);

    int udpSocket;
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket == -1) {
        cerr << "Socket creation failed: " << strerror(errno) << "..." << endl;
        return 1;
    }
    // zoned entries
    unordered_map<string, array<uint8_t,4>> zone = {
        {"google.com", {142,250,193,78}}
    };

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

    stringstream ss(argv[2]);
    string ipArg, portArg;
    getline(ss, ipArg, ':');
    getline(ss, portArg, ':');
    int port = stoi(portArg);

    sockaddr_in resolverAddr;
    resolverAddr.sin_family = AF_INET;
    resolverAddr.sin_port = htons(port);
    inet_pton(AF_INET, ipArg.c_str(), &resolverAddr.sin_addr);

    char resolverResponse[512];

    while (true) {
        // receiving from client
        bytesRead = recvfrom(udpSocket, buffer, sizeof(buffer), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), &clientAddrLen);
        if (bytesRead == -1) {
            perror("Error receiving data");
            break;
        }

        uint16_t qtype, qclass;
        int endPos;
        string domainName = parseQuestion(reinterpret_cast<uint8_t*>(buffer), 12, qtype, qclass, endPos);

        if(zone.find(domainName) != zone.end()){
            dnsHeader h = parseHeader(reinterpret_cast<uint8_t*>(buffer));
            h.aa = 1;
            h.ancount = 1;
            array<uint8_t,12> headerBytes = serialize(h);
            vector<uint8_t> questionBytes = encodeQuestion(domainName, qtype, qclass);
            vector<uint8_t> answerBytes = encodeAnswer(domainName, qtype, qclass, 60, zone[domainName]);

            vector<uint8_t> zoneResponse;
            zoneResponse.insert(zoneResponse.end(), headerBytes.begin(), headerBytes.end());
            zoneResponse.insert(zoneResponse.end(), questionBytes.begin(), questionBytes.end());
            zoneResponse.insert(zoneResponse.end(), answerBytes.begin(), answerBytes.end());

            cout << "Domain: " << domainName << ", Type: " << qtype << ", Class: " << qclass << ", endPos: " << endPos << endl;


            sendto(udpSocket, zoneResponse.data(), zoneResponse.size(), 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress));
            continue;
        }
        cout << "Domain: " << domainName << ", Type: " << qtype << ", Class: " << qclass << ", endPos: " << endPos << endl;

        // forward query to resolver
        int forwardSocket = socket(AF_INET, SOCK_DGRAM, 0);
        if (forwardSocket == -1) {
            cerr << "Socket creation failed: " << strerror(errno) << "..." << endl;
            return 1;
        }

        if (sendto(forwardSocket, buffer, bytesRead, 0, reinterpret_cast<struct sockaddr*>(&resolverAddr), sizeof(resolverAddr)) == -1) {
            perror("Failed to forward query");
        }

        int resolverBytesRead = recvfrom(forwardSocket, resolverResponse, sizeof(resolverResponse), 0, nullptr, nullptr);
        if (resolverBytesRead == -1) {
            perror("Error receiving data from resolver");
            break;
        }
        close(forwardSocket);

        // relay resolver's reply back to the original client
        if (sendto(udpSocket, resolverResponse, resolverBytesRead, 0, reinterpret_cast<struct sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            perror("Failed to send response");
        }
    }
    close(udpSocket);

    return 0;
}
