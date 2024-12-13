#include <iostream>

#include <gflags/gflags.h>


DEFINE_string(hello_server_ip, "0.0.0.0", "IP address");

DEFINE_int32(hello_server_port, 7777, "HTTP port");


int main(int argc, char* argv[]) {
    std::cout << "nacho\n";
    return 0;
}
