#pragma once

#include <mbgl/util/run_loop.hpp>

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

namespace mbgl {

class CustomRunLoop {
public:
    CustomRunLoop();
    ~CustomRunLoop();

    util::RunLoop* getRunLoop() { return run_loop.get(); }

private:
    std::unique_ptr<util::RunLoop> run_loop;
    std::thread thread;
    bool running = true;
    
    void runThread();
};

} // namespace mbgl
