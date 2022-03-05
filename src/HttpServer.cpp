#include "HttpServer.h"
#include "TaskQueue.h"

#include <chrono>
#include <string_view>

#include <arpa/inet.h>

using namespace std::chrono_literals;

namespace
{
    using ResponsePointer = std::unique_ptr<MHD_Response, decltype(&MHD_destroy_response)>;

    ResponsePointer createResponse(
        const std::string& content = {}
    )
    {
        auto* response = MHD_create_response_from_buffer(
            content.size(),
            const_cast<char*>(content.c_str()),
            MHD_RESPMEM_MUST_COPY
        );

        MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "text/plain");

        return ResponsePointer{ response, &MHD_destroy_response };
    }
}

HttpServer::HttpServer(
    const LoggerFactory& loggerFactory,
    TaskQueue& taskQueue,
    Configuration config
)
    : _log{ loggerFactory.create("HttpServer") }
    , _taskQueue{ taskQueue }
    , _config{ std::move(config) }
{
    _log.info(
        "Created: config={}",
        toString(_config)
    );
}

void HttpServer::start()
{
    if (_mhdHandle) {
        _log.warn("MHD daemon already started");
        return;
    }

    _log.debug("Starting MHD daemon");

    static const auto accessPolicyCallback = [](
        void* cls,
        const struct sockaddr* addr,
        socklen_t addrlen
    ) -> MHD_Result {
        if (auto* self = reinterpret_cast<HttpServer*>(cls)) {
            return self->onMhdAccessPolicy(addr, addrlen);
        }

        return MHD_NO;
    };

    static const auto defaultAccessHandler = [](
        void* cls,
        struct MHD_Connection* connection,
        const char* url,
        const char* method,
        const char* version,
        const char* uploadData,
        size_t *uploadDataSize,
        void** conCls
    ) -> MHD_Result {
        if (auto* self = reinterpret_cast<HttpServer*>(cls)) {
            return self->onMhdDefaultAccessHandler(
                connection,
                url,
                method,
                version,
                uploadData,
                uploadDataSize,
                conCls
            );
        }

        return MHD_NO;
    };

    _mhdHandle = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_DEBUG,
        _config.port,
        accessPolicyCallback,
        this,
        defaultAccessHandler,
        this,
        MHD_OPTION_END
    );

    if (!_mhdHandle) {
        _log.warn("Can't start MHD daemon");
    }
}

void HttpServer::stop()
{
    if (!_mhdHandle) {
        _log.warn("MHD daemon already stopped");
        return;
    }

    _log.debug("Stopping MHD daemon");

    MHD_stop_daemon(_mhdHandle);
}

MHD_Result HttpServer::onMhdAccessPolicy(
    const struct sockaddr* addr,
    socklen_t addrlen
) {
    std::string address;

    switch (addr->sa_family) {
        case AF_INET: {
            const auto* inAddr = reinterpret_cast<const struct sockaddr_in*>(addr);
            char s[INET_ADDRSTRLEN] = { 0 };
            inet_ntop(AF_INET, &inAddr->sin_addr, s, INET_ADDRSTRLEN);
            address = s;
            break;
        }

        case AF_INET6: {
            const auto* inAddr = reinterpret_cast<const struct sockaddr_in6*>(addr);
            char s[INET6_ADDRSTRLEN] = { 0 };
            inet_ntop(AF_INET6, &inAddr->sin6_addr, s, INET6_ADDRSTRLEN);
            address = s;
            break;
        }

        default:
            break;
    }

    _log.debug("onMhdAccessPolicy: address={}", address);

    return MHD_YES;
}

MHD_Result HttpServer::onMhdDefaultAccessHandler(
    struct MHD_Connection* connection,
    const char* url,
    const char* method,
    const char* version,
    const char* uploadData,
    size_t *uploadDataSize,
    void** conCls
) {
    _log.debug("onMhdDefaultAccessHandler: url={}, method={}, vesion={}", url, method, version);

    const std::string_view vUrl{ url };
    const std::string_view vMethod{ method };

    if (vMethod != MHD_HTTP_METHOD_GET) {
        _log.warn("Method not allowed: {}", vMethod);

        MHD_queue_response(
            connection,
            MHD_HTTP_METHOD_NOT_ALLOWED,
            createResponse().get()
        );

        return MHD_YES;
    }

    const auto endpointAllowed = std::any_of(
        std::cbegin(_config.allowedEndpoints),
        std::cend(_config.allowedEndpoints),
        [this, &vUrl](const std::string& ep) {
            return vUrl == ep;
        }
    );

    if (!endpointAllowed) {
        _log.warn("Endpoint not allowed: {}", vUrl);

        MHD_queue_response(
            connection,
            MHD_HTTP_NOT_FOUND,
            createResponse().get()
        );

        return MHD_YES;
    }

    if (!_requestHandler) {
        _log.warn("Request handler not registered");

        MHD_queue_response(
            connection,
            MHD_HTTP_INTERNAL_SERVER_ERROR,
            createResponse("Request handler not registered\n").get()
        );

        return MHD_YES;
    }

    decltype(Request::_requestHeaders) requestHeaders;

    Request request{
        url,
        std::move(requestHeaders)
    };

    auto requestFuture = request._promise.get_future();

    _taskQueue.push("HttpServerRequestHandler", [this, &request](auto&) {
        _requestHandler(request);
    });

    if (const auto status = requestFuture.wait_for(5s); status == std::future_status::timeout) {
        _log.warn("Request handler timed out");

        MHD_queue_response(
            connection,
            MHD_HTTP_INTERNAL_SERVER_ERROR,
            createResponse("Request handler timed out\n").get()
        );

        return MHD_YES;
    }

    MHD_queue_response(
        connection,
        request._responseCode,
        createResponse(request._responseContent).get()
    );

    return MHD_YES;
}

std::string toString(const HttpServer::Configuration& config)
{
    return fmt::format(
        "{{port={}}}",
        config.port
    );
}