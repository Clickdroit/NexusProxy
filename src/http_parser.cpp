#include "http_parser.hpp"
#include <sstream>
#include <algorithm>

namespace nexus {

HttpRequest parse_http_request(const std::string& raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    // Parse request line: METHOD URI HTTP/version
    if (!std::getline(stream, line)) return req;
    if (!line.empty() && line.back() == '\r') line.pop_back();

    std::istringstream req_line(line);
    req_line >> req.method >> req.uri >> req.version;
    if (req.method.empty() || req.uri.empty()) return req;

    // Parse headers
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break; // blank line = end of headers

        auto colon = line.find(':');
        if (colon == std::string::npos) continue;

        std::string name  = line.substr(0, colon);
        std::string value = line.substr(colon + 1);

        // Trim leading whitespace from value
        value.erase(0, value.find_first_not_of(" \t"));

        // Normalize header name to lowercase
        std::transform(name.begin(), name.end(), name.begin(), ::tolower);
        req.headers[name] = value;
    }

    // Remaining stream is body
    req.body = std::string(std::istreambuf_iterator<char>(stream), {});
    req.valid = true;
    return req;
}

std::string rewrite_proxy_headers(
    const std::string& raw_request,
    const std::string& client_ip,
    const std::string& backend_host,
    int backend_port)
{
    HttpRequest req = parse_http_request(raw_request);
    if (!req.valid) return raw_request;

    // Overwrite / inject proxy headers
    req.headers["host"]             = backend_host + ":" + std::to_string(backend_port);
    req.headers["x-forwarded-for"]  = client_ip;
    req.headers["x-real-ip"]        = client_ip;
    req.headers["x-forwarded-proto"] = "http";

    // Rebuild the raw request
    std::ostringstream out;
    out << req.method << " " << req.uri << " " << req.version << "\r\n";
    for (const auto& [key, val] : req.headers) {
        // Capitalize first letter of each word for canonical form
        std::string hkey = key;
        bool cap = true;
        for (char& c : hkey) {
            if (cap) { c = static_cast<char>(::toupper(c)); cap = false; }
            if (c == '-') cap = true;
        }
        out << hkey << ": " << val << "\r\n";
    }
    out << "\r\n" << req.body;
    return out.str();
}

} // namespace nexus
