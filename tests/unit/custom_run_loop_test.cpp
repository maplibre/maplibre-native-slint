#include "../test_utils.hpp"
#include "custom_run_loop.hpp"
#include <gtest/gtest.h>
#include <chrono>
#include <thread>

class CustomRunLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        run_loop = std::make_unique<mbgl::CustomRunLoop>();
    }
    
    void TearDown() override {
        run_loop.reset();
    }
    
    std::unique_ptr<mbgl::CustomRunLoop> run_loop;
};

TEST_F(CustomRunLoopTest, ConstructorDestructor) {
    // Test basic construction and destruction
    auto rl = std::make_unique<mbgl::CustomRunLoop>();
    EXPECT_NE(rl, nullptr);
    
    // Test that we can get the run loop
    auto* loop = rl->getRunLoop();
    EXPECT_NE(loop, nullptr);
    
    rl.reset();
}

TEST_F(CustomRunLoopTest, GetRunLoopNotNull) {
    auto* loop = run_loop->getRunLoop();
    EXPECT_NE(loop, nullptr);
}

TEST_F(CustomRunLoopTest, MultipleGetRunLoopCallsReturnSame) {
    auto* loop1 = run_loop->getRunLoop();
    auto* loop2 = run_loop->getRunLoop();
    
    EXPECT_EQ(loop1, loop2);
}

TEST_F(CustomRunLoopTest, RunLoopSurvivesMultipleCalls) {
    // Test that the run loop remains valid across multiple calls
    for (int i = 0; i < 10; ++i) {
        auto* loop = run_loop->getRunLoop();
        EXPECT_NE(loop, nullptr);
        
        // Small delay to ensure thread safety
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TEST_F(CustomRunLoopTest, DestructionCleansUpProperly) {
    auto* loop = run_loop->getRunLoop();
    EXPECT_NE(loop, nullptr);
    
    // Reset should clean up the run loop thread properly
    EXPECT_NO_THROW(run_loop.reset());
}

TEST_F(CustomRunLoopTest, MultipleRunLoopsIndependent) {
    auto run_loop2 = std::make_unique<mbgl::CustomRunLoop>();
    
    auto* loop1 = run_loop->getRunLoop();
    auto* loop2 = run_loop2->getRunLoop();
    
    EXPECT_NE(loop1, nullptr);
    EXPECT_NE(loop2, nullptr);
    EXPECT_NE(loop1, loop2);
    
    run_loop2.reset();
}

// Note: More sophisticated tests would require:
// 1. Testing actual task scheduling and execution
// 2. Testing thread safety
// 3. Testing proper cleanup of pending tasks
// 4. Performance tests
// These would require access to the internal implementation or mock interfaces