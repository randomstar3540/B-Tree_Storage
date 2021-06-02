#ifndef BTREESTORE_H
#define BTREESTORE_H

#include <stdint.h>
#include <stddef.h>

#define KEY_INDEX 4

#define NORMAL 0
#define LEAF 1
#define SERIAL_OFFSET 1
#define PUSH_OFFSET 2
#define SPLIT_OFFSET 1
#define KEY_INDEX_OFFSET 1
#define KEY_PUSH_OFFSET 1
#define KEY_SPLIT_OFFSET 1
#define KEY_REMOVE_OFFSET 1

#define BITS_BYTE 8

#define CHILD_INDEX_OFFSET 2
#define CHILD_PUSH_OFFSET 1
#define CHILD_SIZE_OFFSET 1
#define CHILD_SPLIT_OFFSET 2
#define CHILD_SWAP_OFFSET 1
#define LEFT_CHILD_OFFSET 1
#define RIGHT_CHILD_OFFSET 1

#define TRUE 1
#define FALSE 0

#define NODE_EXPORT_SIZE 1
struct info {
    uint32_t size;
    uint32_t key[4];
    uint64_t nonce;
    void * data;
};

struct node {
    uint16_t num_keys;
    uint32_t * keys;
};

typedef struct key_node{
    int64_t chunk_size;
    uint64_t key_val;
    uint32_t size;
    uint32_t key[4];
    uint64_t nonce;
    void * data;
}key_node;

typedef struct tree_node{
    uint16_t current_size;
    uint8_t status;
    key_node ** key;
    struct tree_node * parent;
    struct tree_node ** children;
}tree_node;

typedef struct header{
    uint16_t branching;
    uint16_t minimum;
    uint8_t processor;
    struct tree_node * root;
    uint64_t key_size;
    uint64_t node_size;
    pthread_mutex_t mut;
}header;

void * init_store(uint16_t branching, uint8_t n_processors);

void close_store(void * helper);

int btree_insert(uint32_t key, void * plaintext, size_t count, uint32_t encryption_key[4], uint64_t nonce, void * helper);

int btree_retrieve(uint32_t key, struct info * found, void * helper);

int btree_decrypt(uint32_t key, void * output, void * helper);

int btree_delete(uint32_t key, void * helper);

uint64_t btree_export(void * helper, struct node ** list);

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]);

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]);

void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], uint64_t nonce, uint64_t * cipher, uint32_t num_blocks);

void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], uint64_t nonce, uint64_t * plain, uint32_t num_blocks);

void debug(void * helper);

#endif
