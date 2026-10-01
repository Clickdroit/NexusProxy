#pragma once
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
    constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
    inline void close_socket(socket_t s) { closesocket(s); }
    inline void platform_init() {
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
            throw std::runtime_error("WSAStartup failed");
    }
    inline void platform_cleanup() { WSACleanup(); }
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    using socket_t = int;
    constexpr socket_t INVALID_SOCKET_VALUE = -1;
    inline void close_socket(socket_t s) { close(s); }
    inline void platform_init() {}
    inline void platform_cleanup() {}
#endif

#include <string>
#include <stdexcept>

namespace nexus {

/// Creates a non-blocking TCP connection to host:port.
/// Returns connected socket or INVALID_SOCKET_VALUE on failure.
socket_t connect_to(const std::string& host, int port, int timeout_ms = 2000);

/// Creates and binds a TCP listening socket on the given port.
socket_t make_listener(int port, int backlog = 128);

} // namespace nexus
