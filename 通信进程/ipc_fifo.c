#include "ipc_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
    int fd_r;
    int fd_w;
    char path_r[256];
    char path_w[256];
    int is_server;
} fifo_priv_t;

static ipc_context_t* fifo_init(const char* name, int is_server) {
    fifo_priv_t* priv = calloc(1, sizeof(fifo_priv_t));
    if (!priv) return NULL;

    priv->is_server = is_server;
    snprintf(priv->path_r, sizeof(priv->path_r), "/tmp/%s_%s", name, is_server ? "c2s" : "s2c");
    snprintf(priv->path_w, sizeof(priv->path_w), "/tmp/%s_%s", name, is_server ? "s2c" : "c2s");

    if (is_server) {
        mkfifo(priv->path_r, 0666);
        mkfifo(priv->path_w, 0666);
    }

    // 统一打开顺序以防死锁：Server 先读后写，Client 先写后读
    if (is_server) {
        priv->fd_r = open(priv->path_r, O_RDONLY);
        priv->fd_w = open(priv->path_w, O_WRONLY);
    } else {
        priv->fd_w = open(priv->path_w, O_WRONLY);
        priv->fd_r = open(priv->path_r, O_RDONLY);
    }

    if (priv->fd_r < 0 || priv->fd_w < 0) {
        free(priv);
        return NULL;
    }

    ipc_context_t* ctx = malloc(sizeof(ipc_context_t));
    ctx->priv = priv;
    return ctx;
}

static int fifo_send(ipc_context_t* ctx, const void* data, size_t len) {
    fifo_priv_t* priv = ctx->priv;
    return write(priv->fd_w, data, len);
}

static int fifo_recv(ipc_context_t* ctx, void* buf, size_t len) {
    fifo_priv_t* priv = ctx->priv;
    return read(priv->fd_r, buf, len);
}

static void fifo_cleanup(ipc_context_t* ctx) {
    fifo_priv_t* priv = ctx->priv;
    close(priv->fd_r);
    close(priv->fd_w);
    if (priv->is_server) {
        unlink(priv->path_r);
        unlink(priv->path_w);
    }
    free(priv);
    free(ctx);
}

static const struct ipc_ops fifo_ops = {
    .init = fifo_init,
    .send = fifo_send,
    .recv = fifo_recv,
    .cleanup = fifo_cleanup
};

const struct ipc_ops* get_fifo_ops() { return &fifo_ops; }
