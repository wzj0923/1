#include "ipc_interface.h"

extern const struct ipc_ops* get_fifo_ops();
extern const struct ipc_ops* get_msgq_ops();
extern const struct ipc_ops* get_shm_ops();
extern const struct ipc_ops* get_unix_ops();

const struct ipc_ops* get_ipc_ops(ipc_type_t type) {
    switch (type) {
        case IPC_TYPE_FIFO: return get_fifo_ops();
        case IPC_TYPE_MSG_QUEUE: return get_msgq_ops();
        case IPC_TYPE_SHM: return get_shm_ops();
        case IPC_TYPE_UNIX_SOCKET: return get_unix_ops();
        default: return NULL;
    }
}
