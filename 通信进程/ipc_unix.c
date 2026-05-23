#include "ipc_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
    int fd;
    int is_server;
    char path[256];
} unix_priv_t;

static ipc_context_t* unix_init(const char* name, int is_server) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) return NULL;

    unix_priv_t* priv = malloc(sizeof(unix_priv_t));
    priv->fd = fd;
    priv->is_server = is_server;
    snprintf(priv->path, sizeof(priv->path), "/tmp/%s.sock", name);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, priv->path, sizeof(addr.sun_path) - 1);

    if (is_server) {
        unlink(priv->path);
        if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1 || listen(fd, 5) == -1) {
            free(priv);
            return NULL;
        }
        int client_fd = accept(fd, NULL, NULL);
        close(fd); // 监听套接字可以关了，本实验只测一对一
        priv->fd = client_fd;
    } else {
        while (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            usleep(100000);
        }
    }

    ipc_context_t* ctx = malloc(sizeof(ipc_context_t));
    ctx->priv = priv;
    return ctx;
}

static int unix_send(ipc_context_t* ctx, const void* data, size_t len) {
    unix_priv_t* priv = ctx->priv;
    return send(priv->fd, data, len, 0);
}

static int unix_recv(ipc_context_t* ctx, void* buf, size_t len) {
    unix_priv_t* priv = ctx->priv;
    return recv(priv->fd, buf, len, 0);
}

static void unix_cleanup(ipc_context_t* ctx) {
    unix_priv_t* priv = ctx->priv;
    close(priv->fd);
    if (priv->is_server) unlink(priv->path);
    free(priv);
    free(ctx);
}

static const struct ipc_ops unix_ops = {
    .init = unix_init,
    .send = unix_send,
    .recv = unix_recv,
    .cleanup = unix_cleanup
};

const struct ipc_ops* get_unix_ops() { return &unix_ops; }
