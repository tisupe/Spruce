#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <array>
#include <string>
#include <sys/types.h>
#include <vector>
#include <cstdint>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

struct dnsHeader {
    uint16_t id = 1234;

    bool qr = 1;
    uint8_t opcode = 0;
    bool aa = 0;
    bool tc = 0;
    bool rd = 0;
    bool ra = 0;
    uint16_t z = 0;
    uint8_t rcode = 0;

    uint16_t qdcount = 1;
    uint16_t ancount = 1;
    uint16_t nscount = 0;
    uint16_t arcount = 0;
};

inline std::array<uint8_t, 12> serialize(const dnsHeader &h) {
    uint16_t flags = (h.qr << 15) | (h.opcode << 11) | (h.aa << 10) | (h.tc << 9) | (h.rd << 8) | (h.ra << 7) | (h.z << 4) | (h.rcode << 0);
    std::array<uint8_t, 12> result;
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

inline std::vector<uint8_t> encodeDomain(const std::string &domain) {
    std::vector<std::string> labels;
    std::stringstream ss(domain);
    std::string label;

    while(getline(ss,label,'.')){
        labels.push_back(label);
    }

    std::vector<uint8_t> result;

    for (const std::string &label : labels) {
        result.push_back(static_cast<uint8_t>(label.size()));  // length byte
        for (char c : label) {
            result.push_back(static_cast<uint8_t>(c));  // each character as a byte
        }
    }
    result.push_back(0);
    return result;
}

inline std::vector<uint8_t> encodeQuestion(const std::string &domain, uint16_t qtype, uint16_t qclass) {
    std::vector<uint8_t> result;

    std::vector<uint8_t> encodeName = encodeDomain(domain);
    result.insert(result.end(), encodeName.begin(), encodeName.end());

    result.push_back(qtype/256);
    result.push_back(qtype%256);

    result.push_back(qclass/256);
    result.push_back(qclass%256);

    return result;
}

inline std::vector<uint8_t> encodeAnswer(const std::string &domain, uint16_t atype, uint16_t aclass, uint32_t ttl, const std::array<uint8_t,4> &ip) {
    std::vector<uint8_t> result;
    std::vector<uint8_t> encodeName = encodeDomain(domain);

    result.insert(result.end(), encodeName.begin(), encodeName.end());

    result.push_back(atype/256);
    result.push_back(atype%256);

    result.push_back(aclass/256);
    result.push_back(aclass%256);

    result.push_back((ttl >> 24) & 0xFF);
    result.push_back((ttl >> 16) & 0xFF);
    result.push_back((ttl >> 8) & 0xFF);
    result.push_back(ttl & 0xFF);

    result.push_back(0);
    result.push_back(4);

    result.insert(result.end(),ip.begin(),ip.end());

    return result;
}
