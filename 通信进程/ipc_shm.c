#include "ipc_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <string.h>
#include <unistd.h>

#define SHM_SIZE (1024 * 1024 * 2) // 2MB

typedef struct {
    int shmid;
    int semid;
    void* addr;
    int is_server;
} shm_priv_t;

// 简单的信号量 P/V 操作
static void sem_op(int semid, int op) {
    struct sembuf sb = {0, op, 0};
    semop(semid, &sb, 1);
}

static ipc_context_t* shm_init(const char* name, int is_server) {
    key_t key = ftok("/tmp", name[0] + 10);
    int shmid = shmget(key, SHM_SIZE, 0666 | (is_server ? IPC_CREAT : 0));
    if (shmid == -1) return NULL;

    int semid = semget(key, 1, 0666 | (is_server ? IPC_CREAT : 0));
    if (semid == -1) return NULL;

    if (is_server) semctl(semid, 0, SETVAL, 0); // 初始为 0

    void* addr = shmat(shmid, NULL, 0);
    if (addr == (void*)-1) return NULL;

    shm_priv_t* priv = malloc(sizeof(shm_priv_t));
    priv->shmid = shmid;
    priv->semid = semid;
    priv->addr = addr;
    priv->is_server = is_server;

    ipc_context_t* ctx = malloc(sizeof(ipc_context_t));
    ctx->priv = priv;
    return ctx;
}

static int shm_send(ipc_context_t* ctx, const void* data, size_t len) {
    shm_priv_t* priv = ctx->priv;
    memcpy(priv->addr, data, len);
    sem_op(priv->semid, 1); // V 操作，通知对方
    return (int)len;
}

static int shm_recv(ipc_context_t* ctx, void* buf, size_t len) {
    shm_priv_t* priv = ctx->priv;
    sem_op(priv->semid, -1); // P 操作，等待通知
    memcpy(buf, priv->addr, len);
    return (int)len;
}

static void shm_cleanup(ipc_context_t* ctx) {
    shm_priv_t* priv = ctx->priv;
    shmdt(priv->addr);
    if (priv->is_server) {
        shmctl(priv->shmid, IPC_RMID, NULL);
        semctl(priv->semid, 0, IPC_RMID);
    }
    free(priv);
    free(ctx);
}

static const struct ipc_ops shm_ops = {
    .init = shm_init,
    .send = shm_send,
    .recv = shm_recv,
    .cleanup = shm_cleanup
};

const struct ipc_ops* get_shm_ops() { return &shm_ops; }
