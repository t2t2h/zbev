// Hook.m - getaddrinfo (DNS override) + socket tuning + registration of every hook - AFLATOON
#import <Foundation/Foundation.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <stdatomic.h>
#include "Shared.h"
#include "DNS.h"
#include "Rebind.h"
#include "SpeedCore.h"

#ifndef TCP_KEEPALIVE
#define TCP_KEEPALIVE 0x10
#endif
#ifndef SO_TRAFFIC_CLASS
#define SO_TRAFFIC_CLASS 0x1086
#endif

volatile int ap_dnsOn = 0;
const char * volatile ap_dnsServer = "1.1.1.1";
volatile unsigned long ap_dnsResolved = 0;
char ap_dnsLastHost[64] = "-";

static NSMutableDictionary<NSString *, NSString *> *gCache;

void ap_dns_clear_cache(void) {
    @synchronized (gCache) { [gCache removeAllObjects]; }
}

// ---- net.boost: socket tuning on this app's OWN TCP sockets. NOT a VPN. ----
volatile int ap_netBoostOn = 0;
static _Atomic int ap_boostedCount = 0;
static int (*orig_socket)(int, int, int) = NULL;

static void ap_boost_fd(int fd, int domain) {
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof one);
    int idle = 30;
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPALIVE, &idle, sizeof idle);
    int rb = 512 * 1024, sb = 256 * 1024;
    setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rb, sizeof rb);
    setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sb, sizeof sb);
    int tc = 700;
    setsockopt(fd, SOL_SOCKET, SO_TRAFFIC_CLASS, &tc, sizeof tc);
    if (domain == AF_INET) { int tos = 0x10; setsockopt(fd, IPPROTO_IP, IP_TOS, &tos, sizeof tos); }
    atomic_fetch_add(&ap_boostedCount, 1);
}

static int my_socket(int domain, int type, int proto) {
    int fd = orig_socket(domain, type, proto);
    if (fd >= 0 && ap_netBoostOn && type == SOCK_STREAM && (domain == AF_INET || domain == AF_INET6))
        ap_boost_fd(fd, domain);
    return fd;
}

int ap_net_boosted_count(void) { return atomic_load(&ap_boostedCount); }

static int is_ip_literal(const char *s) {
    struct in_addr a4; struct in6_addr a6;
    return inet_pton(AF_INET, s, &a4) == 1 || inet_pton(AF_INET6, s, &a6) == 1;
}

static int (*orig_getaddrinfo)(const char *, const char *, const struct addrinfo *, struct addrinfo **);

static int my_getaddrinfo(const char *node, const char *service,
                          const struct addrinfo *hints, struct addrinfo **res) {
    if (!ap_dnsOn || !node || !*node || is_ip_literal(node) || strcmp(node, "localhost") == 0)
        return orig_getaddrinfo(node, service, hints, res);

    const char *server = ap_dnsServer;
    NSString *key = [NSString stringWithFormat:@"%s|%s", server, node];
    NSString *ip = nil;
    @synchronized (gCache) { ip = gCache[key]; }
    if (!ip) {
        struct in_addr a;
        if (ap_dns_resolve(node, server, 53, &a)) {
            char buf[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &a, buf, sizeof buf);
            ip = [NSString stringWithUTF8String:buf];
            @synchronized (gCache) { gCache[key] = ip; }
        }
    }
    if (ip) {
        int rc = orig_getaddrinfo(ip.UTF8String, service, hints, res);
        if (rc == 0) {
            __atomic_fetch_add(&ap_dnsResolved, 1, __ATOMIC_RELAXED);
            strlcpy(ap_dnsLastHost, node, sizeof ap_dnsLastHost);
            return rc;
        }
    }
    return orig_getaddrinfo(node, service, hints, res);
}

// ONE registry for every hook of the dylib (DNS, sockets, audio, clocks), one dyld callback.
void ap_install_hooks(void) {
    gCache = [NSMutableDictionary new];
    dlopen("/System/Library/Frameworks/AudioToolbox.framework/AudioToolbox", RTLD_NOW);

    ap_hook_symbol("getaddrinfo", (void *)my_getaddrinfo, (void **)&orig_getaddrinfo, 0);
    ap_hook_symbol("AudioUnitRender", (void *)ap_AudioUnitRender, (void **)&ap_orig_AudioUnitRender, 0);
    ap_hook_symbol("socket", (void *)my_socket, (void **)&orig_socket, 0);

    MBSpeedInstall();            // Uses the same runtime hook adapter for clock symbols.
    ap_rebind_start();           // No-op for direct runtime hooks; installs fallback imports otherwise.
}
