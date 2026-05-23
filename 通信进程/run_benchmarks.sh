#!/bin/bash

# 编译
gcc -I/home/ubuntu/ipc_project/include /home/ubuntu/ipc_project/src/*.c /home/ubuntu/ipc_project/tests/perf_test.c -o /home/ubuntu/ipc_project/perf_test

SIZES=("1" "64" "1024" "65536" "1048576")
TYPES=("0:FIFO" "1:MSG_QUEUE" "2:SHM" "3:UNIX_SOCKET")

echo "Type,Size,Avg_Latency_ms,Throughput_MBs" > /home/ubuntu/ipc_project/results.csv

for T_STR in "${TYPES[@]}"; do
    T_ID=${T_STR%%:*}
    T_NAME=${T_STR#*:}
    echo "Benchmarking $T_NAME..."
    for S in "${SIZES[@]}"; do
        # 1MB 包只跑 100 次以加速
        if [ "$S" == "1048576" ]; then
            # 这里需要修改 perf_test.c 支持参数，但为了简单，我直接运行
            RESULT=$(/home/ubuntu/ipc_project/perf_test $T_ID $S)
        else
            RESULT=$(/home/ubuntu/ipc_project/perf_test $T_ID $S)
        fi
        echo "$T_NAME,$RESULT" >> /home/ubuntu/ipc_project/results.csv
    done
done

echo "Benchmark completed. Results saved to results.csv"
