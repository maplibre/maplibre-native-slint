#pragma once

#include <mbgl/storage/file_source.hpp>
#include <mbgl/storage/resource_options.hpp>
#include <mbgl/util/client_options.hpp>
#include <mbgl/util/run_loop.hpp>

#include <cpr/cpr.h>

#include <memory>
#include <string>

namespace mbgl {

class CustomFileSource : public FileSource {
public:
    CustomFileSource();
    ~CustomFileSource() override;

    std::unique_ptr<AsyncRequest> request(const Resource&, Callback) override;
    bool canRequest(const Resource&) const override;
    
    void setResourceOptions(ResourceOptions options) override;
    ResourceOptions getResourceOptions() override;
    void setClientOptions(ClientOptions options) override;
    ClientOptions getClientOptions() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
    ResourceOptions resourceOptions;
    ClientOptions clientOptions;
};

} // namespace mbgl
