#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "btreestore.h"

void encrypt_key_cpy(uint32_t* key_to ,uint32_t encryption_key[4]){
    for (int i = 0; i < KEY_INDEX; ++i) {
        key_to[i] = encryption_key[i];
    }
}
void dfs_debug(tree_node * node, uint64_t* counter){
    if (node == NULL){
        return;
    }
    key_node * key_ptr;
    printf("NODE: status %d, SIZE: %d\n",node->status, node->current_size);
    printf("ADDR:%p\n",node);
    printf("CONT:%lu\n",*counter);
    for (int i = 0; i < node->current_size; ++i) {
        key_ptr = *(node->key + i);
        printf("Key :%d VALUE :%lu \n",i, key_ptr->key_val);
    }

    for (int i = 0; i < node->current_size + CHILD_SIZE_OFFSET; ++i) {
        printf("Child :%d ADDR :%p \n",i, *(node->children + i));
    }
    printf("\n\n");
    for (int i = 0; i < node->current_size + CHILD_SIZE_OFFSET; ++i) {
        dfs_debug(*(node->children + i), counter);
    }
}
void debug(void * helper){
    header * head = helper;
    uint64_t counter_num = 0;
    uint64_t * counter = &counter_num;
    printf("================DEBUG INFORMATION=================\n");
    printf("=================HEAD INFORMATION=================\n");
    printf("Branching: %d ",head->branching);
    printf("Processor: %d \n",head->processor);
    printf("KEYS: %lu \n",head->key_size);
    printf("NODES: %lu \n",head->node_size);
    printf("=====================END==========================\n");
    dfs_debug(head->root,counter);
    printf("=====================END==========================\n");
}
/*
 * Allocating space for a new node
 * And initialize the space for storing keys and child
 */
tree_node * make_new_node(header * head){
    tree_node * new = (tree_node *)malloc(sizeof(tree_node));

    if(new == NULL){
        //Check if malloc failed
        return NULL;
    }

    memset(new,0,sizeof(tree_node));

    /*
     * Malloc Space for storing children node pointers and key pointers
     * Reserved space for one more key to simplify insert
     */
    void * children = calloc(head->branching +CHILD_SIZE_OFFSET,
                             sizeof(tree_node*));
    void * keys = calloc(head->branching,sizeof(key_node*));

    if(children == NULL || keys == NULL){
        //Check if malloc failed
        return NULL;
    }

    new->children = children;
    new->key = keys;
    new->current_size = 0;
    new->parent = NULL;
    new->status = NORMAL;

    return new;
}

int node_add_key(tree_node * node, key_node * key,
                 tree_node * child, header * head){
    if (node->current_size >= head->branching){
        return 1;
    }

    key_node * key_ptr;
    int32_t child_index = 1;
    int32_t key_index = 0;
    for(int i = 0; i < node->current_size; i++){
        key_ptr = *(node->key + i);

        if (key->key_val > key_ptr->key_val){
            child_index = i + CHILD_INDEX_OFFSET;
            key_index = i + KEY_INDEX_OFFSET;

        }else if (key->key_val < key_ptr->key_val){
            break;

        }else{
            // Check if the key already exists in the tree.
            return 1;
        }
    }

    key_node ** key_dest;
    key_node ** key_src;
    uint64_t size;
    tree_node ** child_dest;
    tree_node ** child_src;

    /*
     * Push the keys backward to insert the key pointer
     * Keep them in order
     *
     * dest: right next to the place that we want to insert the key
     * src: the place we want to insert the key
     * size: number of keys we want to push
     */
    key_dest = node->key + key_index + KEY_PUSH_OFFSET;
    key_src = node->key + key_index;
    size = sizeof(key_node*) * (node->current_size - key_index);
    memmove(key_dest,key_src,size);


    /*
     * Push the child backward to insert the child within the inserting key
     * Keep them in order
     *
     * dest: right next to the place that we want to insert the child
     * src: the place we want to insert the key
     * size: number of children we want to push
     */
    child_dest = node->children + child_index + CHILD_PUSH_OFFSET;
    child_src = node->children + child_index;
    size = sizeof(key_node*) *
           (node->current_size - child_index + CHILD_PUSH_OFFSET);
    memmove(child_dest, child_src, size);

    *(node->key + key_index) = key;
    *(node->children + child_index) = child;
    node->current_size += 1;
    return 0;
}



int node_remove_key(tree_node * node, uint32_t key, header * head){

    if (node->status != LEAF){
        return 1;
    }

    key_node * key_ptr;

    key_node ** key_dest;
    key_node ** key_src;
    uint64_t size;

    for(int i = 0; i < node->current_size; i++){
        key_ptr = *(node->key + i);
        if(key_ptr->key_val == key){
            *(node->key + i) = NULL;

            /*
             * Push the child forward and overwrite the key we want to remove
             *
             * dest: the key we want to remove
             * src: the key next to the key we want to remove
             * size: number of key we want to push forward
             */
            key_dest = node->key + i;
            key_src = node->key + i + KEY_REMOVE_OFFSET;
            size = sizeof(key_node*) *
                   (node->current_size - i + KEY_REMOVE_OFFSET);
            memmove(key_dest,key_src,size);

            free(key_ptr->data);
            free(key_ptr);
            key_ptr = NULL;

            node->current_size -= 1;
            head->key_size -= 1;

            return 0;
        }
    }

    return 1;
}

int swap_and_remove(tree_node * node, uint32_t key, header * head){
    key_node * key_ptr;
    key_node * max_key_ptr;
    tree_node * left_child;
    uint64_t max_key_index;

    if (node->status != LEAF){
        for(int i = 0; i < node->current_size; i++){

            key_ptr = *(node->key + i);
            left_child = *(node->children + i);

            if(key_ptr->key_val == key){
                if (left_child->current_size == 0){
                    return 1;
                }

                max_key_index = left_child->current_size - CHILD_SWAP_OFFSET;
                max_key_ptr = *(left_child->key + max_key_index);

                *(left_child->key + max_key_index) = key_ptr;
                *(node->key + i) = max_key_ptr;

                swap_and_remove(left_child, key, head);
                return 0;
            }
        }
        return 1;
    }

    node_remove_key(node, key, head);
    return 0;
}


/*
 * Split the old node into itself and the new node
 *
 * First copy the keys and children for the new node
 * Then, clear the keys and children that split our in the old node
 */
int split_node(tree_node * old, tree_node * new,
               int median, header * head){

    new->status = old->status;
    new->current_size = 0;
    new->parent = old->parent;

    key_node ** key_dest;
    key_node ** key_src;
    uint64_t size;
    tree_node ** child_dest;
    tree_node ** child_src;

    //Content of the new node start from the key next to the median
    int split_index = median + SPLIT_OFFSET;

    /*
     * Copy ALL key pointers that will split into the new node
     *
     * dest: key pointer space in the new node
     * src: the key next to the median key
     * size: current size of the node - index of src
     */
    key_dest = new->key;
    key_src = old->key + split_index;
    size = sizeof(key_node*) * (old->current_size - split_index);
    memcpy(key_dest, key_src, size);

    /*
     * Copy ALL child pointers that will split into the new node
     *
     * dest: child pointer space in the new node
     * src: the child pointer has the same index next to the median key
     * size: current size of the node - median index
     */
    child_dest = new->children;
    child_src = old->children + split_index;
    size = sizeof(tree_node*) * (old->current_size - median);
    memcpy(child_dest, child_src, size);


    /*
     * Set the key pointer space that split out in the old node to 0
     *
     * src: the median key
     * size: current size of the node - median
     */
    key_src = old->key + median;
    size = sizeof(key_node*) * (old->current_size - median);
    memset(key_src,0, size);

    /*
     * Set the child pointer space that split out in the old node to 0
     *
     * src: child pointer has same the index next to the median key
     * size: current size of the node - median
     */
    child_src = old->children + split_index;
    size = sizeof(key_node*) * (old->current_size - median);
    memset(child_src,0,size);

    new->current_size = old->current_size - split_index;
    old->current_size = median;
    head->node_size += 1;
    return 0;
}

int check_node_overflow(tree_node * node, header * head){
    if(node->current_size >= head->branching){
        int median;
        if (node->current_size % 2 == 0){
            median = node->current_size / 2 - 1;
        }else{
            median = node->current_size / 2;
        }

        key_node * key_median = *(node->key + median);
        tree_node * new_node = make_new_node(head);
        tree_node * parent = node->parent;

        if (new_node == NULL){
            return 1;
        }

        if(parent == NULL){
            //If there's no parent node, which means we are in the root
            //Make a new root node
            tree_node * new_root = make_new_node(head);

            if (new_root == NULL){
                return 1;
            }

            split_node(node,new_node,median,head);


            new_root->status = NORMAL;
            new_root->parent = NULL;

            //Set the parent of the two split nodes to the new root
            new_node->parent = new_root;
            node->parent = new_root;

            //Add the median key to the root and set related children
            *(new_root->children) = node;
            if (node_add_key(new_root,key_median,new_node,head) != 0){
                return 1;
            }

            head->root = new_root;
            head->node_size +=1;

        }else{
            //If a parent node exist

            split_node(node,new_node,median,head);

            //Add the median key to the parent
            //And set the right child to the split new node
            if (node_add_key(parent,key_median,new_node,head) != 0){
                return 1;
            }

            //recursively check if the parent node’s number of keys overflows
            check_node_overflow(parent,head);
        }
    }
}

int check_node_underflow(tree_node * node, header * head){

}


void dfs_free(tree_node * node){

    if (node == NULL){
        return;
    }

    for (int i = 0; i < node->current_size + CHILD_SIZE_OFFSET; ++i) {
        dfs_free(*(node->children + i));
    }

    key_node * key_ptr;
    for (int i = 0; i < node->current_size; ++i) {
        key_ptr = *(node->key + i);
        free(key_ptr->data);
        free(key_ptr);
    }

    free(node->children);
    free(node->key);
    free(node);
}

/*
 * Initialize the b_tree data structure
 * Malloc a block of memory to store the config variables
 * Return:
 * On success: return the pointer of the block of memory
 * Failed: return NULL
 */
void * init_store(uint16_t branching, uint8_t n_processors) {

    header * tree = (header *)malloc(sizeof(header));
    memset(tree,0,sizeof(header));

    if(tree == NULL){
        //Check if malloc failed
        return NULL;
    }

    tree->branching = branching;
    tree->processor = n_processors;

    tree_node * root = make_new_node(tree);

    // Initialize the node
    root->status = LEAF;
    tree->root = root;
    tree->key_size = 0;
    tree->node_size = 1;
    return tree;
}



void close_store(void * helper) {
    header * head = helper;

    dfs_free(head->root);
    free(head);
}

/*
 * Insert a key
 */
int btree_insert(uint32_t key, void * plaintext, size_t count,
                 uint32_t encryption_key[4], uint64_t nonce, void * helper) {

    header * head = helper;
    tree_node * current_node = head->root;
    tree_node * next_node = NULL;

    // Check if the key already exists in the tree.
    struct info check;
    if(btree_retrieve(key,&check,helper) == 0){
        return 1;
    }

    while (current_node != NULL || current_node->current_size > 0){
        key_node * key_ptr;

        //on default, pick the smallest child
        next_node = *(current_node->children);

        for(int i = 0; i < current_node->current_size; i++){
            key_ptr = *(current_node->key + i);

            if (key_ptr->key_val < key){
                next_node = *(current_node->children + i + 1);

            }else if (key_ptr->key_val > key){
                break;

            }else{
                // Check if the key already exists in the tree.
                return 1;
            }
        }

        if (current_node->status == LEAF){
            break;
        }

        current_node = next_node;
        next_node = NULL;
    }

    if(current_node == NULL){
        return 1;
    }

    //Allocating space for the new key and data
    key_node * new_key = (key_node *)malloc(sizeof(key_node));
    void * data = malloc(count);

    if (new_key == NULL || data == NULL){
        return 1;
    }

    memset(new_key,0,sizeof(key_node));
    memset(data,0,count);
    memcpy(data,plaintext,count);

    encrypt_tea_ctr(plaintext,encryption_key,nonce,data,count);

    new_key->key_val = key;
    new_key->size = count;
    new_key->data = data;
    new_key->nonce = nonce;

    encrypt_key_cpy(new_key->key, encryption_key);

    node_add_key(current_node,new_key,NULL,head);

    head->key_size += 1;
    check_node_overflow(current_node,head);
    return 0;
}

int btree_retrieve(uint32_t key, struct info * found, void * helper) {
    header * head = helper;
    tree_node * current_node = head->root;
    tree_node * next_node = NULL;

    while (current_node != NULL || current_node->current_size > 0){
        key_node * key_ptr;

        //on default, pick the smallest child
        next_node = *(current_node->children);

        for(int i = 0; i < current_node->current_size; i++){
            key_ptr = *(current_node->key + i);

            if (key_ptr->key_val < key){
                next_node = *(current_node->children + i + 1);

            }else if (key_ptr->key_val > key){
                break;

            }else{
                found->nonce = key_ptr->nonce;
                found->data = key_ptr->data;
                found->size = key_ptr->size;
                encrypt_key_cpy(found->key,key_ptr->key);
                return 0;
            }
        }

        if (current_node->status == LEAF){
            break;
        }

        current_node = next_node;
        next_node = NULL;
    }
    return 1;
}

int btree_decrypt(uint32_t key, void * output, void * helper) {
    header * head = helper;
    tree_node * current_node = head->root;
    tree_node * next_node = NULL;

    while (current_node != NULL || current_node->current_size > 0){
        key_node * key_ptr;

        //on default, pick the smallest child
        next_node = *(current_node->children);

        for(int i = 0; i < current_node->current_size; i++){
            key_ptr = *(current_node->key + i);

            if (key_ptr->key_val < key){
                next_node = *(current_node->children + i + 1);

            }else if (key_ptr->key_val > key){
                break;

            }else{
                decrypt_tea_ctr(key_ptr->data,key_ptr->key,key_ptr->nonce,
                                output,key_ptr->size);
                return 0;
            }
        }

        if (current_node->status == LEAF){
            break;
        }

        current_node = next_node;
        next_node = NULL;
    }
    return 1;
}

int btree_delete(uint32_t key, void * helper) {
    // Your code here
    // Check if the key already exists in the tree.
    header * head = helper;
    tree_node * current_node = head->root;
    tree_node * next_node = NULL;

    struct info check;
    if(btree_retrieve(key,&check,helper) == 1){
        return 1;
    }

    while (current_node != NULL || current_node->current_size > 0){
        key_node * key_ptr;

        //on default, pick the smallest child
        next_node = *(current_node->children);

        for(int i = 0; i < current_node->current_size; i++){
            key_ptr = *(current_node->key + i);

            if (key_ptr->key_val > key){
                next_node = *(current_node->children + i + 1);

            }else if (key_ptr->key_val < key){
                break;

            }else{

                return 0;
            }
        }

        if (current_node->status == LEAF){
            break;
        }

        current_node = next_node;
        next_node = NULL;
    }

    return 1;
}


void dfs_export(tree_node * node, struct node ** list, uint64_t* counter){

    if (node == NULL){
        return;
    }

    struct node * first = *list;
    struct node * export_node = first + *counter;
    uint32_t * keys = (uint32_t *)calloc(node->current_size,sizeof(uint32_t));
    key_node * key_ptr;
    for (int i = 0; i < node->current_size; ++i) {
        key_ptr = *(node->key + i);
        *(keys + i) = key_ptr->key_val;
    }

    export_node->keys = keys;
    export_node->num_keys = node->current_size;

    if (node->status == LEAF){
        return;
    }

    for (int i = 0; i < node->current_size + CHILD_SIZE_OFFSET; ++i) {
        *counter += 1;
        dfs_export(*(node->children + i), list, counter);
    }
}


uint64_t btree_export(void * helper, struct node ** list) {
    header * head = helper;
    *list = calloc(head->node_size,sizeof(struct node));

    if(*list == NULL){
        return 0;
    }
    uint64_t counter_num = 0;
    uint64_t * counter = &counter_num;
    dfs_export(head->root,list,counter);


    return head->node_size;
}

void encrypt_tea(uint32_t plain[2], uint32_t cipher[2], uint32_t key[4]) {
    uint32_t sum = 0;
    uint32_t delta = 0x9E3779B9;
    uint64_t tmp[6] = {0};
    cipher[0] = plain[0];
    cipher[1] = plain[1];

    for (int i = 0; i < 1024; ++i) {
        sum += delta;
        tmp[0] = (cipher[1]<<4) + key[0];
        tmp[1] = (cipher[1] + sum);
        tmp[2] = (cipher[1] >> 5) + key[1];
        cipher[0] = (cipher[0] + (tmp[0] ^ tmp[1] ^ tmp[2]));
        tmp[3] = (cipher[0]<<4) + key[2];
        tmp[4] = (cipher[0] + sum);
        tmp[5] = (cipher[0] >> 5) + key[3];
        cipher[1] = (cipher[1] + (tmp[3] ^ tmp[4] ^ tmp[5]));
    }
}

void decrypt_tea(uint32_t cipher[2], uint32_t plain[2], uint32_t key[4]) {
    uint32_t sum = 0xDDE6E400;
    uint32_t delta = 0x9E3779B9;
    uint64_t tmp[6] = {0};

    for (int i = 0; i < 1024; ++i) {
        tmp[3] = (cipher[0]<<4) + key[2];
        tmp[4] = (cipher[0] + sum);
        tmp[5] = (cipher[0] >> 5) + key[3];
        cipher[1] = (cipher[1] - (tmp[3] ^ tmp[4] ^ tmp[5]));
        tmp[0] = (cipher[1]<<4) + key[0];
        tmp[1] = (cipher[1] + sum);
        tmp[2] = (cipher[1] >> 5) + key[1];
        cipher[0] = (cipher[0] - (tmp[0] ^ tmp[1] ^ tmp[2]));
        sum -= delta;
    }

    plain[0] = cipher[0];
    plain[1] = cipher[1];
}

void encrypt_tea_ctr(uint64_t * plain, uint32_t key[4], uint64_t nonce,
                     uint64_t * cipher, uint32_t num_blocks) {
    uint64_t tmp1;
    uint64_t tmp2;

    for (int i = 0; i < num_blocks; i++){
        tmp1 = i ^ nonce;
        encrypt_tea((uint32_t*)&tmp1,(uint32_t*)&tmp2,key);
        cipher[i] = plain[i] ^ tmp2;
    }
}

void decrypt_tea_ctr(uint64_t * cipher, uint32_t key[4], uint64_t nonce,
                     uint64_t * plain, uint32_t num_blocks) {
    uint64_t tmp1;
    uint64_t tmp2;

    for (int i = 0; i < num_blocks; i++){
        tmp1 = i ^ nonce;
        encrypt_tea((uint32_t*)&tmp1,(uint32_t*)&tmp2,key);
        plain[i] = cipher[i] ^ tmp2;
    }
}
