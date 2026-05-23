#ifndef IPC_INTERFACE_H
#define IPC_INTERFACE_H

#include <stddef.h>

typedef enum {
    IPC_TYPE_FIFO,
    IPC_TYPE_MSG_QUEUE,
    IPC_TYPE_SHM,
    IPC_TYPE_UNIX_SOCKET
} ipc_type_t;

typedef struct ipc_context ipc_context_t;

struct ipc_ops {
    ipc_context_t* (*init)(const char* name, int is_server);
    int (*send)(ipc_context_t* ctx, const void* data, size_t len);
    int (*recv)(ipc_context_t* ctx, void* buf, size_t len);
    void (*cleanup)(ipc_context_t* ctx);
};

typedef struct ipc_context {
    void* priv; // 指向具体实现的私有数据
    const struct ipc_ops* ops;
} ipc_context_t;

// 工厂函数：根据类型获取 IPC 操作接口
const struct ipc_ops* get_ipc_ops(ipc_type_t type);

#endif
