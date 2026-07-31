#include <sep/protocol.h>
#include <stl_queue.h>
#include <ingress_service.h>

#include <print>

int main()
{
    std::println("Starting Server...");

    STLQueue<New_order> input_buffer;

    IngressService<STLQueue<New_order>> ingress_service(input_buffer);

    ingress_service.run(1234);

    return 0;
}