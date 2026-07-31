#include <sep/protocol.h>
#include <stl_queue.h>
#include <ingress_service.h>
#include <types.h>
#include <sequencer.h>

#include <print>

int main()
{
    std::println("Starting Server...");

    STLQueue<Order> input_buffer;

    Sequencer sequencer = {};

    IngressService<STLQueue<Order>> ingress_service(input_buffer, sequencer);

    ingress_service.run(1234);

    return 0;
}