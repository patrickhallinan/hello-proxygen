#!/usr/bin/env python3

import subprocess
import sys


def run_performance_command(host, port, number_of_requests, payload_size, num_connections):
    host_param            = f"--target_host={host}"
    port_param            = f"--target_port={port}"
    num_connections_param = f"--num_connections={num_connections}"
    num_requests_param    = f"--number_of_requests={number_of_requests}"
    payload_size_param    = f"--payload_size={payload_size}"

    command = [
        "test/perf",
        host_param,
        port_param,
        num_connections_param,
        num_requests_param,
        payload_size_param
    ]

    #print(command)

    # sometimes connect fais so retry
    for tries in range(5):
        if tries > 0:
            print("trying again")
        try:
            result = subprocess.run(command, check=True, text=True, capture_output=True)
            return result.stderr # glog logs to stderr
        except subprocess.CalledProcessError as e:
            print()
            print(f"An error occurred while running the command: {e}")
            print("Error output:", e.stderr)

    sys.exit(1)


def get_value(line):
    position = line.find("]")
    log_message = line[position+1:].strip()

    position = log_message.find(":")
    metric_with_units = log_message[position+1:].strip()

    return metric_with_units.split(" ")[0]


def parse_output(output):
    print(output)

    lines = output.split("\n")


    for i, line in enumerate(lines):
        if "total time" in line:
            break

    lines = lines[i:]

    total_time = float(get_value(lines[0]))
    avg_request_time = float(get_value(lines[3]))
    request_rate = float(get_value(lines[4]))

    print(f"total_time={total_time}, avg_request_time={avg_request_time}, request_rate={request_rate}")
    return (total_time, avg_request_time, request_rate)


if __name__ == '__main__':
    host = "192.168.1.107"
    port = 8080
    number_of_requests = 1000
    payload_size = 1000
    num_connections = 16

    #payloads = [1, 10, 100, 1000, 10000, 100000]

    class Result:
        pass

    results = []
    for num_connections in [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]:
        requests_per_second_list = []
        for i in range(5):
            output = run_performance_command(host, port, number_of_requests, payload_size, num_connections)
            total_time, avg_request_time, requests_per_second = parse_output(output)
            requests_per_second_list.append(requests_per_second)

        print(requests_per_second_list)



        result = Result()
        result.num_connections = num_connections
        result.min = min(requests_per_second_list)
        result.mean = sum(requests_per_second_list) / len(requests_per_second_list)
        result.max = max(requests_per_second_list)

        results.append(result)

        print()
        print("num_connections,min,mean,max")
        for result in results:

            print(f"{result.num_connections},{result.min},{result.mean},{result.max}")
        print()

