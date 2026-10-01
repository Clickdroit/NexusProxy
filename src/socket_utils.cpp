#include "socket_utils.hpp"
#include <stdexcept>
#include <cstring>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <cerrno>
#endif

namespace nexus {

socket_t connect_to(const std::string& host, int port, int timeout_ms) {
    struct addrinfo hints{};
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    int rv = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
    if (rv != 0 || !res) return INVALID_SOCKET_VALUE;

    socket_t sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET_VALUE) { freeaddrinfo(res); return INVALID_SOCKET_VALUE; }

    // Set non-blocking for timeout support
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    fcntl(sock, F_SETFL, O_NONBLOCK);
#endif

    ::connect(sock, res->ai_addr, static_cast<int>(res->ai_addrlen));
    freeaddrinfo(res);

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(sock, &wfds);
    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    if (select(static_cast<int>(sock) + 1, nullptr, &wfds, nullptr, &tv) <= 0) {
        close_socket(sock);
        return INVALID_SOCKET_VALUE;
    }

    // Verify the connection actually succeeded
    int err = 0;
    socklen_t len = sizeof(err);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len);
    if (err != 0) { close_socket(sock); return INVALID_SOCKET_VALUE; }

    // Set back to blocking
#ifdef _WIN32
    mode = 0;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
#endif
    return sock;
}

socket_t make_listener(int port, int backlog) {
    socket_t sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET_VALUE) throw std::runtime_error("socket() failed");

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close_socket(sock);
        throw std::runtime_error("bind() failed on port " + std::to_string(port));
    }
    if (listen(sock, backlog) < 0) {
        close_socket(sock);
        throw std::runtime_error("listen() failed");
    }
    return sock;
}

} // namespace nexus
