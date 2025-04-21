#include "kvstore.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if ENABLE_ARRAY
extern kvs_array_t global_array;
#endif

#if ENABLE_RBTREE
extern kvs_rbtree_t global_rbtree;
#endif

#if ENABLE_HASH
extern kvs_hash_t global_hash;
#endif

void *kvs_malloc(size_t size) {
    return malloc(size);
}

void kvs_free(void *ptr) {
    free(ptr);
}

const char *command[] = {
    "SET", "GET", "DEL", "MOD", "EXIST",
    "RSET", "RGET", "RDEL", "RMOD", "REXIST",
    "HSET", "HGET", "HDEL", "HMOD", "HEXIST"
};

enum {
    KVS_CMD_START = 0,
    // array
    KVS_CMD_SET = KVS_CMD_START,
    KVS_CMD_GET,
    KVS_CMD_DEL,
    KVS_CMD_MOD,
    KVS_CMD_EXIST,
    // rbtree
    KVS_CMD_RSET,
    KVS_CMD_RGET,
    KVS_CMD_RDEL,
    KVS_CMD_RMOD,
    KVS_CMD_REXIST,
    // hash
    KVS_CMD_HSET,
    KVS_CMD_HGET,
    KVS_CMD_HDEL,
    KVS_CMD_HMOD,
    KVS_CMD_HEXIST,

    KVS_CMD_COUNT,
};

const char *response[] = {

};

// 去除字符串首尾的空白字符
void trim(char *str) {
    if (str == NULL) return;
    size_t len = strlen(str);
    if (len == 0) return;

    // 去除开头的空白字符
    while (*str && (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n')) {
        str++;
    }

    // 去除结尾的空白字符
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        *end = '\0';
        end--;
    }

    // 移动字符串
    if (str != str) {
        memmove(str, str, strlen(str) + 1);
    }
}

// 按 # 分割命令
int kvs_split_token(char *msg, char *tokens[]) {
    if (msg == NULL || tokens == NULL) return -1;

    int idx = 0;
    char *token = strtok(msg, "#");

    while (token != NULL) {
        trim(token); // 去除首尾空白字符
        if (strlen(token) > 0) {
            tokens[idx++] = token;
        }
        token = strtok(NULL, "#");
    }

    return idx;
}

// SET Key Value
// tokens[0] : SET
// tokens[1] : Key
// tokens[2] : Value

int kvs_filter_protocol(char **tokens, int count, char *response) {
    if (tokens[0] == NULL || count == 0 || response == NULL) return -1;

    int cmd = KVS_CMD_START;
    for (cmd = KVS_CMD_START; cmd < KVS_CMD_COUNT; cmd++) {
        if (strcmp(tokens[0], command[cmd]) == 0) {
            break;
        }
    }

    int length = 0;
    int ret = 0;
    char *key = NULL;
    char *value = NULL;

    // 检查命令参数数量和空值
    switch (cmd) {
        case KVS_CMD_RSET:
            printf("count:%d,token[0]: %s,token[1]:%s,token[2]:%s,token[3]:%sn", count, tokens[0], tokens[1], tokens[2], tokens[3]);
            if (count != 4 || tokens[1] == NULL || tokens[2] == NULL) {
                length = sprintf(response, "ERROR: RSET requires exactly 3 non - empty arguments\r\n");
                return length;
            }
            key = tokens[1];
            value = tokens[2];
            break;
        case KVS_CMD_RGET:
            printf("token[0]: %s,token[1]: %s\n", tokens[0], tokens[1]);
            if (count != 3 || tokens[1] == NULL) {
                length = sprintf(response, "ERROR: RGET requires exactly 2 non - empty arguments\r\n");
                return length;
            }
            key = tokens[1];
            break;
        default:
            if (count > 1) {
                key = tokens[1];
            }
            if (count > 2) {
                value = tokens[2];
            }
            break;
    }

    switch (cmd) {
#if ENABLE_ARRAY
    case KVS_CMD_SET:
        if (key != NULL && value != NULL) {
            ret = kvs_array_set(&global_array, key, value);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_GET:
        if (key != NULL) {
            char *result = kvs_array_get(&global_array, key);
            if (result == NULL) {
                length = sprintf(response, "NO EXIST\r\n");
            } else {
                length = sprintf(response, "%s\r\n", result);
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_DEL:
        if (key != NULL) {
            ret = kvs_array_del(&global_array, key);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_MOD:
        if (key != NULL && value != NULL) {
            ret = kvs_array_mod(&global_array, key, value);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_EXIST:
        if (key != NULL) {
            ret = kvs_array_exist(&global_array, key);
            if (ret == 0) {
                length = sprintf(response, "EXIST\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
#endif
        // rbtree
#if ENABLE_RBTREE
    case KVS_CMD_RSET: {
        // 释放之前的值（如果存在）
        char *old_value = kvs_rbtree_get(&global_rbtree, key);
        if (old_value != NULL) {
            kvs_free(old_value);
        }
        // 复制新值
        char *new_value = strdup(value);
        if (new_value == NULL) {
            length = sprintf(response, "ERROR: Memory allocation failed\r\n");
            return length;
        }
        ret = kvs_rbtree_set(&global_rbtree, key, new_value);
        if (ret < 0) {
            length = sprintf(response, "ERROR\r\n");
            kvs_free(new_value);
        } else if (ret == 0) {
            length = sprintf(response, "OK\r\n");
        } else {
            length = sprintf(response, "EXIST\r\n");
        }
        break;
    }
    case KVS_CMD_RGET: {
        char *result = kvs_rbtree_get(&global_rbtree, key);
        if (result == NULL) {
            length = sprintf(response, "NO EXIST\r\n");
        } else {
            length = sprintf(response, "%s\r\n", result);
        }
        break;
    }
    case KVS_CMD_RDEL:
        if (key != NULL) {
            char *old_value = kvs_rbtree_get(&global_rbtree, key);
            if (old_value != NULL) {
                kvs_free(old_value);
            }
            ret = kvs_rbtree_del(&global_rbtree, key);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_RMOD:
        if (key != NULL && value != NULL) {
            char *old_value = kvs_rbtree_get(&global_rbtree, key);
            if (old_value != NULL) {
                kvs_free(old_value);
            }
            char *new_value = strdup(value);
            if (new_value == NULL) {
                length = sprintf(response, "ERROR: Memory allocation failed\r\n");
                return length;
            }
            ret = kvs_rbtree_mod(&global_rbtree, key, new_value);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
                kvs_free(new_value);
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_REXIST:
        if (key != NULL) {
            ret = kvs_rbtree_exist(&global_rbtree, key);
            if (ret == 0) {
                length = sprintf(response, "EXIST\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
#endif
#if ENABLE_HASH
    case KVS_CMD_HSET:
        if (key != NULL && value != NULL) {
            ret = kvs_hash_set(&global_hash, key, value);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_HGET:
        if (key != NULL) {
            char *result = kvs_hash_get(&global_hash, key);
            if (result == NULL) {
                length = sprintf(response, "NO EXIST\r\n");
            } else {
                length = sprintf(response, "%s\r\n", result);
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_HDEL:
        if (key != NULL) {
            ret = kvs_hash_del(&global_hash, key);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_HMOD:
        if (key != NULL && value != NULL) {
            ret = kvs_hash_mod(&global_hash, key, value);
            if (ret < 0) {
                length = sprintf(response, "ERROR\r\n");
            } else if (ret == 0) {
                length = sprintf(response, "OK\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
    case KVS_CMD_HEXIST:
        if (key != NULL) {
            ret = kvs_hash_exist(&global_hash, key);
            if (ret == 0) {
                length = sprintf(response, "EXIST\r\n");
            } else {
                length = sprintf(response, "NO EXIST\r\n");
            }
        } else {
            length = sprintf(response, "ERROR: INSUFFICIENT ARGUMENTS\r\n");
        }
        break;
#endif
    default:
        length = sprintf(response, "UNKNOWN COMMAND\r\n");
        break;
    }

    return length;
}

/*
 * msg: request message
 * length: length of request message
 * response: need to send
 * @return : length of response
 */

int kvs_protocol(char *msg, int length, char *response) {
    if (msg == NULL || length <= 0 || response == NULL) return -1;

    // 确保 msg 以 '\0' 结尾，避免 strtok 越界读取
    if (length > 0 && msg[length - 1] != '\0') {
        msg[length] = '\0';
    }

    printf("Received request (%d bytes): %.*s\n", length, length, msg);

    char *tokens[KVS_MAX_TOKENS] = {0};
    int count = kvs_split_token(msg, tokens);
    if (count == -1) return -1;

    int response_length = kvs_filter_protocol(tokens, count, response);
    if (response_length > 0) {
        printf("Sending response (%d bytes): %.*s\n", response_length, response_length, response);
    }

    return response_length;
}

int init_kvengine(void) {
#if ENABLE_ARRAY
    memset(&global_array, 0, sizeof(kvs_array_t));
    kvs_array_create(&global_array);
#endif

#if ENABLE_RBTREE
    memset(&global_rbtree, 0, sizeof(kvs_rbtree_t));
    kvs_rbtree_create(&global_rbtree);
#endif

#if ENABLE_HASH
    memset(&global_hash, 0, sizeof(kvs_hash_t));
    kvs_hash_create(&global_hash);
#endif

    return 0;
}

void dest_kvengine(void) {
#if ENABLE_ARRAY
    kvs_array_destory(&global_array);
#endif
#if ENABLE_RBTREE
    kvs_rbtree_destory(&global_rbtree);
#endif
#if ENABLE_HASH
    kvs_hash_destory(&global_hash);
#endif
}

int main(int argc, char *argv[]) {
    if (argc != 2) return -1;

    int port = atoi(argv[1]);

    init_kvengine();

#if (NETWORK_SELECT == NETWORK_REACTOR)
    reactor_start(port, kvs_protocol);  //
#elif (NETWORK_SELECT == NETWORK_PROACTOR)
    ntyco_start(port, kvs_protocol);
#elif (NETWORK_SELECT == NETWORK_NTYCO)
    proactor_start(port, kvs_protocol);
#endif

    dest_kvengine();

    return 0;
}    