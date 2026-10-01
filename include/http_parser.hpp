#pragma once
#include <string>
#include <unordered_map>

namespace nexus {

struct HttpRequest {
    std::string method;  // GET, POST, ...
    std::string uri;
    std::string version; // HTTP/1.1
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    bool valid = false;
};

/// Lightweight HTTP/1.1 request parser (no external deps).
/// Returns a parsed HttpRequest from a raw buffer string.
HttpRequest parse_http_request(const std::string& raw);

/// Rewrites proxy headers on the raw request buffer.
/// Adds X-Forwarded-For, X-Real-IP and rewrites Host to match the backend.
std::string rewrite_proxy_headers(
    const std::string& raw_request,
    const std::string& client_ip,
    const std::string& backend_host,
    int backend_port
);

} // namespace nexus
