#include "HttpServer.h"

#include <arpa/inet.h>

HttpServer::HttpServer(
    const LoggerFactory& loggerFactory,
    Configuration config
)
    : _log{ loggerFactory.create("HttpServer") }
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

    std::string responseContent{ "No Content\r\n" };

    auto* response = MHD_create_response_from_buffer(
        responseContent.size(),
        const_cast<char*>(responseContent.c_str()),
        MHD_RESPMEM_MUST_COPY
    );

    MHD_queue_response(
        connection,
        204,
        response
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