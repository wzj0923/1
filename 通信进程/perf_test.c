#include "ipc_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#define CHUNK_SIZE (32 * 1024) // 32KB chunks

double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void run_server(ipc_type_t type, size_t packet_size, int iterations) {
    const struct ipc_ops* ops = get_ipc_ops(type);
    ipc_context_t* ctx = ops->init("perf_test", 1);
    if (!ctx) { exit(1); }

    char* buf = malloc(packet_size);
    for (int i = 0; i < iterations; i++) {
        size_t received = 0;
        while (received < packet_size) {
            size_t to_recv = (packet_size - received > CHUNK_SIZE) ? CHUNK_SIZE : (packet_size - received);
            int r = ops->recv(ctx, buf + received, to_recv);
            if (r <= 0) break;
            received += r;
        }
        
        size_t sent = 0;
        while (sent < packet_size) {
            size_t to_send = (packet_size - sent > CHUNK_SIZE) ? CHUNK_SIZE : (packet_size - sent);
            int s = ops->send(ctx, buf + sent, to_send);
            if (s <= 0) break;
            sent += s;
        }
    }

    free(buf);
    ops->cleanup(ctx);
}

void run_client(ipc_type_t type, size_t packet_size, int iterations) {
    const struct ipc_ops* ops = get_ipc_ops(type);
    ipc_context_t* ctx = ops->init("perf_test", 0);
    if (!ctx) { exit(1); }

    char* send_buf = malloc(packet_size);
    char* recv_buf = malloc(packet_size);
    memset(send_buf, 'A', packet_size);

    double start = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        size_t sent = 0;
        while (sent < packet_size) {
            size_t to_send = (packet_size - sent > CHUNK_SIZE) ? CHUNK_SIZE : (packet_size - sent);
            int s = ops->send(ctx, send_buf + sent, to_send);
            if (s <= 0) break;
            sent += s;
        }

        size_t received = 0;
        while (received < packet_size) {
            size_t to_recv = (packet_size - received > CHUNK_SIZE) ? CHUNK_SIZE : (packet_size - received);
            int r = ops->recv(ctx, recv_buf + received, to_recv);
            if (r <= 0) break;
            received += r;
        }
    }
    double end = get_time_ms();

    double total_time = end - start;
    double avg_latency = total_time / (iterations * 2);
    double throughput = (double)packet_size * iterations * 2 / (total_time / 1000.0) / (1024 * 1024);

    printf("%zu,%f,%f\n", packet_size, avg_latency, throughput);

    free(send_buf);
    free(recv_buf);
    ops->cleanup(ctx);
}

int main(int argc, char** argv) {
    if (argc < 3) return 1;

    ipc_type_t type = atoi(argv[1]);
    size_t packet_size = atoll(argv[2]);
    int iterations = (packet_size >= 1048576) ? 100 : 1000;

    pid_t pid = fork();
    if (pid == 0) {
        run_server(type, packet_size, iterations);
        exit(0);
    } else {
        sleep(1);
        run_client(type, packet_size, iterations);
        wait(NULL);
    }
    return 0;
}
