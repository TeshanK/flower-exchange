//
// Created by teshk on 8/1/26.
//

#include "ingress.h"
#include "sequencer.h"
#include "stl_queue.h"
#include "systypes.h"
#include "registry.h"
#include "map_orderbook.h"
#include "egress.h"
#include "matching_engine.h"
#include <print>
#include <atomic>
#include <chrono>
#include <csignal>
#include <thread>

namespace {
std::atomic<bool> g_running{true};

void handle_signal(int) {
    g_running.store(false);
}
} // namespace

int main() {
    std::println("Starting server...");

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    STLQueue<Order> input_buffer;
    STLQueue<Report> output_buffer;
    Sequencer sequencer = {};

    IngressService<STLQueue<Order>> ingress(input_buffer, sequencer);
    InstrumentRegistry<MapOrderBook> registry;
    MatchingEngine matching_engine(input_buffer, output_buffer, registry);
    EgressService egress(output_buffer);

    std::thread ingress_thread([&] {
        ingress.run(1234);
    });
    ingress_thread.detach();

    std::println("Event loop running...");
    while (g_running.load()) {
        const bool processed_order = matching_engine.step();
        egress.drain();

        if (!processed_order) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::println("Server shutdown");
    return 0;
}
