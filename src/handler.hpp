#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <array>
#include <string>
#include <sys/types.h>
#include <vector>
#include <cstdint>
#include <sstream>
#include <netinet/in.h>

struct dnsHeader {
    uint16_t id = 0;

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
    result[0] = ((h.id >> 8) & 0xFF);
    result[1] = (h.id & 0xFF);
    result[2] = ((flags >> 8) & 0xFF);
    result[3] = (flags & 0xFF);
    result[4] = ((h.qdcount >> 8) & 0xFF);
    result[5] = (h.qdcount & 0xFF);
    result[6] = ((h.ancount >> 8) & 0xFF);
    result[7] = (h.ancount & 0xFF);
    result[8] = ((h.nscount >> 8) & 0xFF);
    result[9] = (h.nscount & 0xFF);
    result[10] = ((h.arcount >> 8) & 0xFF);
    result[11] = (h.arcount & 0xFF);

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

    result.push_back((qtype >> 8) & 0xFF);
    result.push_back(qtype & 0xFF);

    result.push_back((qclass >> 8) & 0xFF);
    result.push_back(qclass & 0xFF);

    return result;
}

inline std::vector<uint8_t> encodeAnswer(const std::string &domain, uint16_t atype, uint16_t aclass, uint32_t ttl, const std::array<uint8_t,4> &ip) {
    std::vector<uint8_t> result;
    std::vector<uint8_t> encodeName = encodeDomain(domain);

    result.insert(result.end(), encodeName.begin(), encodeName.end());

    result.push_back((atype >> 8) & 0xFF);
    result.push_back(atype & 0xFF);
    result.push_back((aclass >> 8) & 0xFF);
    result.push_back(aclass & 0xFF);

    result.push_back((ttl >> 24) & 0xFF);
    result.push_back((ttl >> 16) & 0xFF);
    result.push_back((ttl >> 8) & 0xFF);
    result.push_back(ttl & 0xFF);

    result.push_back(0);
    result.push_back(4);

    result.insert(result.end(),ip.begin(),ip.end());

    return result;
}

inline dnsHeader parseHeader(const uint8_t *buffer) {
    dnsHeader h;

    h.id = (buffer[0] << 8) | buffer[1];

    h.qdcount = (buffer[4] << 8) | buffer[5];
    h.ancount = (buffer[6] << 8) | buffer[7];
    h.nscount = (buffer[8] << 8) | buffer[9];
    h.arcount = (buffer[10] << 8) | buffer[11];

    return h;
}

inline std::string parseDomain(const uint8_t *buffer, int iniPos, int &endPos) {
    int pos = iniPos;
    std::string domain;
    bool jumped = false;
    int realEndPos = 0;

    while (buffer[pos] != 0) {
        uint8_t len = buffer[pos];

        if((len & 0xC0) == 0xC0) {
            if(!jumped) {
                realEndPos = pos + 2;
                jumped = true;
            }
            uint16_t offset = ((len & 0x3F) << 8) | buffer[pos + 1];
            pos = offset;
            continue;
        }

        pos++;
        for (int i = 0; i < len;i++) {
            domain += static_cast<char>(buffer[pos]);
            pos++;
        }
        if (buffer[pos] != 0 && (buffer[pos] & 0xC0) != 0xC0) {
            domain += ".";
        }
    }
    pos++;
    endPos = jumped ? realEndPos : pos;

    return domain;
}

inline std::string parseQuestion(const uint8_t *buffer, int iniPos, uint16_t &qtype, uint16_t &qclass, int &endPos) {
    std::string domain = parseDomain(buffer, iniPos, endPos);
    qtype = (buffer[endPos] << 8) | buffer[endPos + 1];
    qclass = (buffer[endPos + 2] << 8) | buffer[endPos + 3];

    endPos += 4;
    return domain;
}
