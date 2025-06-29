#include "custom_run_loop.hpp"

namespace mbgl {

CustomRunLoop::CustomRunLoop() {
    run_loop = std::make_unique<util::RunLoop>();
    thread = std::thread(&CustomRunLoop::runThread, this);
}

CustomRunLoop::~CustomRunLoop() {
    running = false;
    if (run_loop) {
        run_loop->stop();
    }
    if (thread.joinable()) {
        thread.join();
    }
}

void CustomRunLoop::runThread() {
    if (run_loop) {
        run_loop->run();
    }
}

} // namespace mbgl
