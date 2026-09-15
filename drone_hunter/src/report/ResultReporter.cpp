#include "report/ResultReporter.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <sstream>
#include <thread>

#include "Log.h"

namespace {

// "http://host[:port]" -> {host, port}; no path expected in the base URL we use.
std::pair<std::string, uint16_t> parseBaseUrl(const std::string& baseUrl) {
    std::string rest = baseUrl;
    const std::string prefix = "http://";
    if (rest.rfind(prefix, 0) == 0) {
        rest = rest.substr(prefix.size());
    }

    const auto colon = rest.find(':');
    if (colon == std::string::npos) {
        return {rest, 80};
    }
    return {rest.substr(0, colon), static_cast<uint16_t>(std::stoul(rest.substr(colon + 1)))};
}

int connectWithTimeout(const std::string& host, uint16_t port, int timeoutSec) {
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* resolved = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &resolved) != 0 || !resolved) {
        return -1;
    }

    const int fd = socket(resolved->ai_family, resolved->ai_socktype, resolved->ai_protocol);
    if (fd < 0) {
        freeaddrinfo(resolved);
        return -1;
    }

    const int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    const int rc = connect(fd, resolved->ai_addr, resolved->ai_addrlen);
    freeaddrinfo(resolved);

    if (rc == 0) {
        fcntl(fd, F_SETFL, flags);
        return fd;
    }
    if (errno != EINPROGRESS) {
        close(fd);
        return -1;
    }

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);
    timeval tv{timeoutSec, 0};

    const int selRc = select(fd + 1, nullptr, &writeSet, nullptr, &tv);
    if (selRc <= 0) {
        close(fd);
        return -1;
    }

    int soError = 0;
    socklen_t len = sizeof soError;
    getsockopt(fd, SOL_SOCKET, SO_ERROR, &soError, &len);
    if (soError != 0) {
        close(fd);
        return -1;
    }

    fcntl(fd, F_SETFL, flags);
    return fd;
}

bool sendAll(int fd, const std::string& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const ssize_t n = send(fd, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

// Reads until the peer closes the connection (we always send "Connection: close"),
// bounded by an overall time budget.
std::string recvUntilClosed(int fd, int timeoutSec) {
    std::string data;
    char buf[4096];
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeoutSec);

    while (true) {
        const auto remaining = deadline - std::chrono::steady_clock::now();
        const auto remainingUs = std::chrono::duration_cast<std::chrono::microseconds>(remaining).count();
        if (remainingUs <= 0) {
            break;
        }

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(fd, &readSet);
        timeval tv{};
        tv.tv_sec = static_cast<time_t>(remainingUs / 1000000);
        tv.tv_usec = static_cast<suseconds_t>(remainingUs % 1000000);

        const int selRc = select(fd + 1, &readSet, nullptr, nullptr, &tv);
        if (selRc <= 0) {
            break;  // timeout or error
        }

        const ssize_t n = recv(fd, buf, sizeof buf, 0);
        if (n <= 0) {
            break;  // peer closed or error
        }
        data.append(buf, static_cast<std::size_t>(n));
    }
    return data;
}

}  // namespace

ResultReporter::ResultReporter(std::string baseUrl, std::string apiKey, int maxAttempts,
                                int retryDelaySec, int connectTimeoutSec, int readTimeoutSec)
    : apiKey_(std::move(apiKey)),
      maxAttempts_(maxAttempts),
      retryDelaySec_(retryDelaySec),
      connectTimeoutSec_(connectTimeoutSec),
      readTimeoutSec_(readTimeoutSec) {
    std::tie(host_, port_) = parseBaseUrl(baseUrl);
}

ResultReporter::HttpResponse ResultReporter::request(const std::string& method, const std::string& path,
                                                       const std::string& body) const {
    HttpResponse response;

    const int fd = connectWithTimeout(host_, port_, connectTimeoutSec_);
    if (fd < 0) {
        return response;  // sent stays false -> caller treats it like a timeout
    }

    std::ostringstream req;
    req << method << ' ' << path << " HTTP/1.1\r\n"
        << "Host: " << host_ << "\r\n"
        << "x-api-key: " << apiKey_ << "\r\n"
        << "Connection: close\r\n";
    if (!body.empty()) {
        req << "Content-Type: application/json\r\n"
            << "Content-Length: " << body.size() << "\r\n";
    }
    req << "\r\n" << body;

    if (!sendAll(fd, req.str())) {
        close(fd);
        return response;
    }

    const std::string raw = recvUntilClosed(fd, readTimeoutSec_);
    close(fd);

    const auto headerEnd = raw.find("\r\n\r\n");
    if (raw.rfind("HTTP/", 0) != 0 || headerEnd == std::string::npos) {
        return response;  // no usable response within the timeout
    }

    // "HTTP/1.1 200 OK" -> 200
    const auto codeStart = raw.find(' ') + 1;
    response.statusCode = std::atoi(raw.c_str() + codeStart);
    response.body = raw.substr(headerEnd + 4);
    response.sent = true;
    return response;
}

bool ResultReporter::verifyStored(const std::string& testId, const std::string& studentId) const {
    const auto resp = request("GET", "/api/dz12/results/" + testId + "/" + studentId, "");
    if (!resp.sent || resp.statusCode != 200) {
        return false;
    }
    try {
        return nlohmann::json::parse(resp.body).value("found", false);
    } catch (const std::exception&) {
        return false;
    }
}

ResultReporter::Outcome ResultReporter::reportTest(const std::string& studentId, const std::string& testId,
                                                    const nlohmann::json& simulation) const {
    Outcome outcome;
    outcome.testId = testId;

    nlohmann::json payload;
    payload["studentId"] = studentId;
    payload["testId"] = testId;
    payload["simulation"] = simulation;
    const std::string body = payload.dump();

    for (outcome.attempts = 1; outcome.attempts <= maxAttempts_; ++outcome.attempts) {
        const auto resp = request("POST", "/api/dz12/results", body);

        if (resp.sent && (resp.statusCode == 200 || resp.statusCode == 201)) {
            outcome.status = verifyStored(testId, studentId) ? Status::Success : Status::Unverified;
            if (outcome.status == Status::Unverified) {
                outcome.message = "сервер прийняв, GET-перевірка не підтвердила";
            }
            return outcome;
        }

        if (resp.sent && (resp.statusCode == 400 || resp.statusCode == 401)) {
            outcome.status = Status::ClientError;
            outcome.message = "HTTP " + std::to_string(resp.statusCode) + ": " + resp.body;
            return outcome;
        }

        // 503, інший неочікуваний код або тиша (таймаут) — варто повторити.
        outcome.message = resp.sent ? "HTTP " + std::to_string(resp.statusCode) : "таймаут / немає відповіді";
        LOG("ResultReporter: спроба " << outcome.attempts << " для " << testId << " невдала (" << outcome.message << ")");

        if (outcome.attempts < maxAttempts_) {
            std::this_thread::sleep_for(std::chrono::seconds(retryDelaySec_));
        }
    }

    outcome.attempts = maxAttempts_;
    outcome.status = Status::ServerError;
    return outcome;
}
