#include "ipc_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>

#define MSG_TYPE_S2C 1
#define MSG_TYPE_C2S 2

typedef struct {
    int msgid;
    int is_server;
} msgq_priv_t;

struct msg_buf {
    long mtype;
    char mtext[1]; // 变长数组
};

static ipc_context_t* msgq_init(const char* name, int is_server) {
    key_t key = ftok("/tmp", name[0]);
    int msgid = msgget(key, 0666 | (is_server ? IPC_CREAT : 0));
    if (msgid == -1) return NULL;

    msgq_priv_t* priv = malloc(sizeof(msgq_priv_t));
    priv->msgid = msgid;
    priv->is_server = is_server;

    ipc_context_t* ctx = malloc(sizeof(ipc_context_t));
    ctx->priv = priv;
    return ctx;
}

static int msgq_send(ipc_context_t* ctx, const void* data, size_t len) {
    msgq_priv_t* priv = ctx->priv;
    struct msg_buf* msg = malloc(sizeof(long) + len);
    msg->mtype = priv->is_server ? MSG_TYPE_S2C : MSG_TYPE_C2S;
    memcpy(msg->mtext, data, len);
    int ret = msgsnd(priv->msgid, msg, len, 0);
    free(msg);
    return ret == 0 ? (int)len : -1;
}

static int msgq_recv(ipc_context_t* ctx, void* buf, size_t len) {
    msgq_priv_t* priv = ctx->priv;
    struct msg_buf* msg = malloc(sizeof(long) + len);
    long type = priv->is_server ? MSG_TYPE_C2S : MSG_TYPE_S2C;
    int ret = msgrcv(priv->msgid, msg, len, type, 0);
    if (ret >= 0) memcpy(buf, msg->mtext, ret);
    free(msg);
    return ret;
}

static void msgq_cleanup(ipc_context_t* ctx) {
    msgq_priv_t* priv = ctx->priv;
    if (priv->is_server) msgctl(priv->msgid, IPC_RMID, NULL);
    free(priv);
    free(ctx);
}

static const struct ipc_ops msgq_ops = {
    .init = msgq_init,
    .send = msgq_send,
    .recv = msgq_recv,
    .cleanup = msgq_cleanup
};

const struct ipc_ops* get_msgq_ops() { return &msgq_ops; }
