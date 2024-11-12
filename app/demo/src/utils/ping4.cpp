/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <regex>

#define LOG(fmt, ...) printf("%s: " fmt "\n", __func__, ##__VA_ARGS__)

enum {
    DEFDATALEN = 56,
    MAXIPLEN = 60,
    MAXICMPLEN = 76,
    MAXWAIT = 2,
};

static uint16_t inet_cksum(const void *ptr, int nleft) {
    const uint16_t *addr = (const uint16_t *)ptr;

    unsigned sum = 0;
    while (nleft > 1) {
        sum += *addr++;
        nleft -= 2;
    }

    if (nleft == 1) {
        sum += *(uint8_t *)addr;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);

    return (uint16_t)~sum;
}

static int __ping4(const char *ip, int timeout /* seconds */) {
    int sock = socket(AF_INET, SOCK_RAW, 1 /* 1 == ICMP */);
    if (sock < 0) {
        LOG("create icmp socket fail, %s", strerror(errno));
        return -1;
    }

    int c;
    struct icmp *pkt;
    char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];
    memset(&packet, 0, sizeof(packet));
    pkt = (struct icmp *)&packet;

    const uint16_t pid = getpid();
    const uint16_t seq = 88;

    struct sockaddr_in saddr;
    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr(ip);

    pkt->icmp_type = ICMP_ECHO;
    pkt->icmp_seq = seq;
    pkt->icmp_id = pid;
    pkt->icmp_cksum = inet_cksum((unsigned short *)pkt, sizeof(packet));

    c = sendto(sock, packet, sizeof(packet), 0, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));
    if (c < 0 || c != sizeof(packet)) {
        //LOG("sendto socket fail, %s", strerror(errno));
        close(sock);
        return -1;
    }

    struct sockaddr_in faddr;
    socklen_t slen = sizeof(faddr);
    int now = 0;
    while (1) {
        if (timeout >= 0) {
            if (now > timeout) {
                break;
            }
        }

        struct timeval tv = {MAXWAIT, 0};
        now += tv.tv_sec;

        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        int ret = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (ret > 0) {
            c = recvfrom(sock, packet, sizeof(packet), 0, (struct sockaddr *)&faddr, &slen);
            if (c >= 76) { /* ip + icmp */
                struct iphdr *iphdr = (struct iphdr *)packet;
                pkt = (struct icmp *)(packet + (iphdr->ihl << 2)); /* skip ip hdr */
                if (pkt->icmp_type == ICMP_ECHOREPLY && pkt->icmp_id == pid && pkt->icmp_seq == seq) {
                    close(sock);
                    return 0;
                } else {
                    LOG("recvfrom icmp: type %d, id %d (%d), seq %d (%d)", pkt->icmp_type, pkt->icmp_id, pid, pkt->icmp_seq, seq);
                    continue;
                }
            } else {
                // LOG("recvfrom len is %d", c);
                continue;
            }
        } else if (0 == ret) {
            /* timeout */
            continue;
        } else {
            LOG("select fail, %s", strerror(errno));
            break;
        }
    }

    close(sock);
    return -1;
}

int ping4(const char *ip, int timeout) {
    if (!ip) {
        LOG("nil ip addr");
        return -1;
    }

    std::regex re("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
    std::cmatch m;
    if (std::regex_search(ip, m, re)) {
        return __ping4(m[0].str().c_str(), timeout);
    }

    return -1;
}

#if 0
int main(int argc, char const *argv[]) {
    if (argc != 2) {
        printf("usage: ./ping4 192.168.2.10\n");
        return -1;
    }

    const char *ip = argv[1];
    if (0 == ping4(ip, 10)) {
        printf("ping %s success\n", ip);
        return 0;
    } else {
        printf("ping %s timeout\n", ip);
        return -1;
    }
}
#endif
