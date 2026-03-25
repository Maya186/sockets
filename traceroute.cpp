
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

// КОНСТАНТЫ

#define MAX_HOPS 30
#define TIMEOUT_MS 3000
#define PACKETS_PER_HOP 3
#define ICMP_DATA_SIZE 32
#define ICMP_ECHO_REQUEST   8
#define ICMP_ECHO_REPLY     0
#define ICMP_TIME_EXCEEDED  11

// СТРУКТУРЫ

#pragma pack(push, 1)
struct ICMPHeader {
    unsigned char  type;
    unsigned char  code;
    unsigned short checksum;
    unsigned short identifier;
    unsigned short sequence;
};

struct IPHeader {
    unsigned char  ihl : 4, version : 4;
    unsigned char  tos;
    unsigned short total_len;
    unsigned short ident;
    unsigned short frags;
    unsigned char  ttl;
    unsigned char  proto;
    unsigned short checksum;
    unsigned int   src_ip;
    unsigned int   dst_ip;
};
#pragma pack(pop)

// ФУНКЦИИ: Утилиты

// Расчёт контрольной суммы (RFC 1071)
unsigned short calculate_checksum(unsigned short* buffer, unsigned long size) {
    unsigned long sum = 0;
    while (size > 1) {
        sum += *buffer++;
        size -= 2;
    }
    if (size) {
        sum += *(unsigned char*)buffer;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return ~((unsigned short)sum);
}

// Разрешение имени хоста в IP
unsigned int resolve_hostname(const char* hostname) {
    unsigned int ip = inet_addr(hostname);
    if (ip != INADDR_NONE) {
        return ip;
    }
    struct hostent* host = gethostbyname(hostname);
    if (!host) {
        return INADDR_NONE;
    }
    return *(unsigned int*)host->h_addr;
}

// IP в строку
std::string ip_to_string(unsigned int ip) {
    char buffer[16];
    unsigned int ip_host = ntohl(ip);
    sprintf_s(buffer, "%d.%d.%d.%d",
        (ip_host >> 24) & 0xFF,
        (ip_host >> 16) & 0xFF,
        (ip_host >> 8) & 0xFF,
        ip_host & 0xFF
    );
    return std::string(buffer);
}

// Обратный DNS
std::string reverse_dns_lookup(unsigned int ip) {
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    addr.sin_port = 0;

    char hostname[NI_MAXHOST];
    int result = getnameinfo(
        (sockaddr*)&addr, sizeof(addr),
        hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD
    );

    if (result == 0) {
        return std::string(hostname);
    }
    return "";
}

// Вывод справки
void print_usage() {
    printf("Usage: traceroute [options] <IP_or_hostname>\n\n");
    printf("Options:\n");
    printf("  -d, --dns    Enable reverse DNS lookup\n\n");
    printf("Examples:\n");
    printf("  traceroute 8.8.8.8\n");
    printf("  traceroute google.com\n");
    printf("  traceroute -d 8.8.8.8\n");
}

// ФУНКЦИИ: ICMP


// Отправка ICMP Echo Request
int send_icmp_echo(SOCKET sock, unsigned int dst_ip, unsigned short seq, unsigned short id) {
    char packet[1024] = { 0 };

    ICMPHeader* icmp = (ICMPHeader*)packet;
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->identifier = htons(id);
    icmp->sequence = htons(seq);

    char* data = packet + sizeof(ICMPHeader);
    for (int i = 0; i < ICMP_DATA_SIZE; i++) {
        data[i] = static_cast<char>(i);
    }

    icmp->checksum = calculate_checksum(
        reinterpret_cast<unsigned short*>(packet),
        sizeof(ICMPHeader) + ICMP_DATA_SIZE
    );

    sockaddr_in dest = {};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = dst_ip;

    return sendto(sock, packet, sizeof(ICMPHeader) + ICMP_DATA_SIZE, 0,
        reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
}

// Получение ответа
int receive_reply(SOCKET sock, unsigned int* reply_ip, unsigned char* icmp_type,
    unsigned char* icmp_code, unsigned short* icmp_seq) {
    char buffer[2048];
    sockaddr_in from;
    int fromlen = sizeof(from);

    int timeout = TIMEOUT_MS;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<char*>(&timeout), sizeof(timeout));

    int len = recvfrom(sock, buffer, sizeof(buffer), 0,
        reinterpret_cast<sockaddr*>(&from), &fromlen);
    if (len == SOCKET_ERROR) {
        return SOCKET_ERROR;
    }

    if (len < 20) return SOCKET_ERROR;

    IPHeader* ip = reinterpret_cast<IPHeader*>(buffer);
    *reply_ip = ip->src_ip;

    int ip_header_len = (ip->ihl * 4);
    if (len < ip_header_len + 8) return SOCKET_ERROR;

    ICMPHeader* icmp = reinterpret_cast<ICMPHeader*>(buffer + ip_header_len);
    *icmp_type = icmp->type;
    *icmp_code = icmp->code;
    *icmp_seq = ntohs(icmp->sequence);

    return len;
}

// ГЛАВНАЯ ФУНКЦИЯ

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    bool reverse_dns = false;
    const char* target = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dns") == 0) {
            reverse_dns = true;
        }
        else {
            target = argv[i];
        }
    }

    if (!target) {
        print_usage();
        return 1;
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    unsigned int dst_ip = resolve_hostname(target);
    if (dst_ip == INADDR_NONE) {
        fprintf(stderr, "Cannot resolve: %s\n", target);
        WSACleanup();
        return 1;
    }

    std::string dst_ip_str = ip_to_string(dst_ip);

    if (inet_addr(target) == INADDR_NONE) {
        printf("Tracing route to %s [%s]\n", target, dst_ip_str.c_str());
    }
    else {
        printf("Tracing route to %s\n", dst_ip_str.c_str());
    }
    printf("Over a maximum of %d hops:\n\n", MAX_HOPS);

    SOCKET sock = WSASocket(AF_INET, SOCK_RAW, IPPROTO_ICMP,
        nullptr, 0, WSA_FLAG_OVERLAPPED);

    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "Raw socket failed: %d\n", WSAGetLastError());
        fprintf(stderr, "Run as Administrator!\n");
        WSACleanup();
        return 1;
    }

    unsigned short icmp_id = static_cast<unsigned short>(GetCurrentProcessId() & 0xFFFF);
    bool target_reached = false;

    for (unsigned char ttl = 1; ttl <= MAX_HOPS; ttl++) {
        printf("%3d  ", ttl);

        setsockopt(sock, IPPROTO_IP, IP_TTL,
            reinterpret_cast<char*>(&ttl), sizeof(ttl));

        double rtts[PACKETS_PER_HOP];
        unsigned int reply_ips[PACKETS_PER_HOP];
        bool success[PACKETS_PER_HOP] = { false };

        for (int probe = 0; probe < PACKETS_PER_HOP; probe++) {
            clock_t start = clock();

            int send_result = send_icmp_echo(sock, dst_ip, probe, icmp_id);

            if (send_result == SOCKET_ERROR) {
                rtts[probe] = -1;
                continue;
            }

            unsigned int reply_ip;
            unsigned char icmp_type, icmp_code;
            unsigned short icmp_seq;
            int recv_result = receive_reply(sock, &reply_ip, &icmp_type, &icmp_code, &icmp_seq);

            clock_t end = clock();
            double rtt = static_cast<double>(end - start) * 1000.0 / CLOCKS_PER_SEC;

            if (recv_result == SOCKET_ERROR) {
                rtts[probe] = -1;
            }
            else {
                rtts[probe] = rtt;
                reply_ips[probe] = reply_ip;
                success[probe] = true;

                if (icmp_type == ICMP_ECHO_REPLY) {
                    target_reached = true;
                }
            }
        }

        for (int probe = 0; probe < PACKETS_PER_HOP; probe++) {
            if (rtts[probe] < 0) {
                printf("   *  ");
            }
            else {
                printf("%4.0f ms  ", rtts[probe]);
            }
        }

        for (int probe = PACKETS_PER_HOP - 1; probe >= 0; probe--) {
            if (success[probe]) {
                std::string ip_str = ip_to_string(reply_ips[probe]);

                if (reverse_dns) {
                    std::string hostname = reverse_dns_lookup(reply_ips[probe]);
                    if (!hostname.empty() && hostname != ip_str) {
                        printf("%s [%s]", hostname.c_str(), ip_str.c_str());
                    }
                    else {
                        printf("%s", ip_str.c_str());
                    }
                }
                else {
                    printf("%s", ip_str.c_str());
                }
                break;
            }
        }

        printf("\n");

        if (target_reached) {
            break;
        }
    }

    printf("\nTrace complete.\n");

    closesocket(sock);
    WSACleanup();

    return 0;
}