#include "custom_file_source.hpp"

#include <cpr/cpr.h>
#include <future>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/thread.hpp>
#include <mbgl/util/timer.hpp>

namespace mbgl {

class CustomFileSource::Impl {
public:
    Impl() = default;

    void request(const Resource& resource, Callback callback) {
        auto future = std::async(std::launch::async, [url = resource.url]() {
            cpr::Response r = cpr::Get(cpr::Url{url});
            Response response;

            if (r.error.code != cpr::ErrorCode::OK) {
                response.error = std::make_unique<Response::Error>(
                    Response::Error::Reason::Connection, r.error.message);
            } else if (r.status_code < 200 || r.status_code >= 300) {
                response.error = std::make_unique<Response::Error>(
                    Response::Error::Reason::Server,
                    "HTTP status code " + std::to_string(r.status_code));
            } else {
                response.data = std::make_shared<std::string>(r.text);
            }
            return response;
        });

        // This is a simplified implementation. A real implementation would
        // manage the future and avoid blocking. For this example, we'll
        // just get the result synchronously.
        callback(future.get());
    }
};

CustomFileSource::CustomFileSource() : impl(std::make_unique<Impl>()) {
}

CustomFileSource::~CustomFileSource() = default;

std::unique_ptr<AsyncRequest> CustomFileSource::request(
    const Resource& resource, Callback callback) {
    impl->request(resource, std::move(callback));
    // A proper implementation would return a request object that can be
    // cancelled.
    return std::make_unique<AsyncRequest>();
}

bool CustomFileSource::canRequest(const Resource& resource) const {
    return resource.kind == Resource::Kind::Style ||
           resource.kind == Resource::Kind::Source ||
           resource.kind == Resource::Kind::Tile ||
           resource.kind == Resource::Kind::Glyphs ||
           resource.kind == Resource::Kind::SpriteImage ||
           resource.kind == Resource::Kind::SpriteJSON;
}

void CustomFileSource::setResourceOptions(ResourceOptions options) {
    resourceOptions = std::move(options);
}

ResourceOptions CustomFileSource::getResourceOptions() {
    return std::move(resourceOptions);
}

void CustomFileSource::setClientOptions(ClientOptions options) {
    clientOptions = std::move(options);
}

ClientOptions CustomFileSource::getClientOptions() {
    return std::move(clientOptions);
}

}  // namespace mbgl
