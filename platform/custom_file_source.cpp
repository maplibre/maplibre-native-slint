#include "custom_file_source.hpp"

#include <atomic>
#include <cpr/cpr.h>
#include <mbgl/storage/response.hpp>
#include <mbgl/util/thread.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace mbgl {

// Concrete implementation of AsyncRequest that supports cancellation.
class CancellableRequest : public AsyncRequest {
public:
    CancellableRequest()
        : cancelled(std::make_shared<std::atomic_bool>(false)) {
    }
    ~CancellableRequest() override {
        cancelled->store(true);
    }
    std::shared_ptr<std::atomic_bool> cancelled;
};

class CustomFileSource::Impl {
public:
    Impl() = default;
    ~Impl() {
        std::lock_guard<std::mutex> lock(threadsMutex);
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void request(const Resource& resource, Callback callback,
                 std::shared_ptr<std::atomic_bool> cancelled) {
        std::lock_guard<std::mutex> lock(threadsMutex);
        threads.emplace_back([url = resource.url,
                              callback = std::move(callback),
                              cancelled = std::move(cancelled)]() {
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

            if (!cancelled->load()) {
                callback(response);
            }
        });
    }

private:
    std::mutex threadsMutex;
    std::vector<std::thread> threads;
};

CustomFileSource::CustomFileSource() : impl(std::make_unique<Impl>()) {
}

CustomFileSource::~CustomFileSource() = default;

std::unique_ptr<AsyncRequest> CustomFileSource::request(
    const Resource& resource, Callback callback) {
    auto req = std::make_unique<CancellableRequest>();
    impl->request(resource, std::move(callback), req->cancelled);
    return req;
}

bool CustomFileSource::canRequest(const Resource& resource) const {
    const std::string& url = resource.url;
    if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
        return false;
    }

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
