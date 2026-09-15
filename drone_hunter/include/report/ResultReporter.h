#pragma once

#include <cstdint>
#include <string>

#include "json.hpp"

class ResultReporter {
public:
    enum class Status { Success, Unverified, ClientError, ServerError, FileMissing };

    struct Outcome {
        std::string testId;
        int attempts = 0;
        Status status = Status::ServerError;
        std::string message;
    };

    ResultReporter(std::string baseUrl, std::string apiKey, int maxAttempts,
                   int retryDelaySec, int connectTimeoutSec, int readTimeoutSec);

    Outcome reportTest(const std::string& studentId, const std::string& testId,
                        const nlohmann::json& simulation) const;

private:
    struct HttpResponse {
        bool sent = false;      // request went out and a response line came back
        int statusCode = 0;
        std::string body;
    };

    HttpResponse request(const std::string& method, const std::string& path,
                          const std::string& body) const;
    bool verifyStored(const std::string& testId, const std::string& studentId) const;

    std::string host_;
    uint16_t port_;
    std::string apiKey_;
    int maxAttempts_;
    int retryDelaySec_;
    int connectTimeoutSec_;
    int readTimeoutSec_;
};
