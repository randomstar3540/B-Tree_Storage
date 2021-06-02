#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>


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
    printf("Parent %p \n",node->parent);
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
    printf("MINIMUM: %d \n",head->minimum);
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

void free_key(key_node * key){
    free(key->data);
    free(key);
    key = NULL;
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
                   (node->current_size - i - KEY_REMOVE_OFFSET);
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

tree_node * swap_and_remove(tree_node * node, uint32_t key, header * head){
    key_node * key_ptr;
    key_node * max_key_ptr;
    tree_node * max_child;
    tree_node * child_ptr;
    tree_node * left_child;
    tree_node * target;
    uint64_t max_key_index;



    if (node->status == LEAF){
        node_remove_key(node, key, head);
        return node;
    }


    for(int i = 0; i < node->current_size; i++){
        key_ptr = *(node->key + i);
        left_child = *(node->children + i);

        if(key_ptr->key_val == key){
            max_child = left_child;
            while (max_child->status != LEAF){
                child_ptr = NULL;
                for (int j = 0; j < max_child->current_size + 1; j ++){
                    if (*(max_child->children + j)!= NULL){
                        child_ptr = *(max_child->children + j);
                    }
                }
                if (child_ptr == NULL){
                    return NULL;
                }
                max_child = child_ptr;
            }

            max_key_ptr = *(max_child->key + max_child->current_size -1);
            max_key_index = max_child->current_size -1;

            *(max_child->key + max_key_index) = key_ptr;
            *(node->key + i) = max_key_ptr;
            node_remove_key(max_child, key, head);
            return max_child;
        }
    }
    return NULL;
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

    /*
     * Update children in the new node
     */
    tree_node  * child_ptr;
    for (int i = 0; i < new->current_size + CHILD_SIZE_OFFSET; i++){
        child_ptr = *(new->children + i);
        if (child_ptr != NULL){
            child_ptr->parent = new;
        }

    }
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

int check_node_underflow(tree_node * target, header * head){

    tree_node * target_parent;
    target_parent = target->parent;
    tree_node * child;
    tree_node * left_child;
    tree_node * right_child;
    key_node * left_key;
    key_node * right_key;
    uint8_t child_found = FALSE;
    uint64_t left_index;
    uint64_t right_index;

    if (target->current_size >= head->minimum){
        return 0;
    }

    for (int i = 0; i < target_parent->current_size + CHILD_SIZE_OFFSET; i++){
        child = *(target_parent->children + i);


        if(i > 0){
            left_key = *(target_parent->key + i - 1);
            left_child = *(target_parent->children + i - LEFT_CHILD_OFFSET);
            left_index = i - 1;
        } else {
            left_key = NULL;
            left_child = NULL;
        }

        if(i < target_parent->current_size){
            right_key = *(target_parent->key + i);
            right_child = *(target_parent->children + i + RIGHT_CHILD_OFFSET);
            right_index = i;
        } else {
            right_key = NULL;
            right_child = NULL;
        }

        if(child == target){
            child_found = TRUE;
            break;
        }

    }

    if(child_found == FALSE){
        return 1;
    }

    if(left_key != NULL && left_child != NULL
       && left_child->current_size > head->minimum){

        uint64_t max_key_index;
        key_node * max_key_ptr;
        tree_node * max_child;

        max_key_index = left_child->current_size - CHILD_SWAP_OFFSET;
        max_key_ptr = *(left_child->key + max_key_index);
        max_child = *(left_child->children + left_child->current_size);

        *(left_child->key + max_key_index) = NULL;
        *(left_child->children + left_child->current_size) = NULL;

        node_add_key(target,left_key,*(target->children),head);
        *(target->children) = max_child;
        *(target_parent->key + left_index) = max_key_ptr;
        left_child->current_size -= 1;
        return 0;
    }


    if (right_key != NULL && right_child != NULL
        && right_child->current_size > head->minimum){

        key_node * min_key_ptr;
        tree_node * min_child;

        min_key_ptr = *(right_child->key);
        min_child = *(right_child->children);

        key_node ** key_dest;
        key_node ** key_src;
        tree_node ** child_dest;
        tree_node ** child_src;
        uint64_t size;

        /*
         * Push the child and key forward and overwrite the key we want to swap
         *
         * dest: the key/child we want to swap
         * src: the key/child next to the key we want to swap
         * size: number of key/child we want to push forward
         */
        key_dest = right_child->key;
        key_src = right_child->key + KEY_REMOVE_OFFSET;
        size = sizeof(key_node*) *
               (right_child->current_size - KEY_REMOVE_OFFSET);
        memmove(key_dest,key_src,size);

        child_dest = right_child->children;
        child_src = right_child->children + KEY_REMOVE_OFFSET;
        size = sizeof(tree_node *) * (right_child->current_size);
        memmove(child_dest,child_src,size);

        node_add_key(target,right_key,min_child,head);
        *(target_parent->key + right_index) = min_key_ptr;
        right_child->current_size -=1;
        return 0;
    }

    if(left_key != NULL && left_child != NULL){

        key_node ** key_dest;
        key_node ** key_src;
        tree_node ** child_dest;
        tree_node ** child_src;
        uint64_t size;

        size = left_child->current_size + CHILD_SIZE_OFFSET;
        for (int i = 0; i < size; ++i) {
            tree_node * child_ptr = *(left_child->children + i);
            if (child_ptr != NULL){
                child_ptr->parent = target;
            }
        }

        /*
         * Push the child and key backward
         *
         * dest: first key/child in target node + size after merge
         * src: first key/child in target node
         * size: number of key/child we want to push forward
         */
        key_dest = target->key
                   + left_child->current_size + KEY_REMOVE_OFFSET;
        key_src = target->key;
        size = sizeof(key_node*) * (target->current_size);
        memmove(key_dest,key_src,size);

        child_dest = target->children
                     + left_child->current_size + KEY_REMOVE_OFFSET;
        child_src = target->children;
        size = sizeof(tree_node *)
               * (target->current_size + KEY_REMOVE_OFFSET);
        memmove(child_dest,child_src,size);

        /*
         * Copy keys and children from the left node
         */
        size = sizeof(key_node*) * (left_child->current_size);
        memcpy(target->key, left_child->key, size);
        size = sizeof(tree_node*) *
               (left_child->current_size + KEY_REMOVE_OFFSET);
        memcpy(target->children,left_child->children,size);
        *(target->key + left_child->current_size) = left_key;

        /*
         * Pull key/child in parent forward
         */
        key_dest = target_parent->key + left_index;
        key_src = target_parent->key + left_index + KEY_REMOVE_OFFSET;
        size = sizeof(key_node*) *
               target_parent->current_size - left_index - KEY_REMOVE_OFFSET;
        memmove(key_dest,key_src,size);

        child_dest = target_parent->children + left_index;
        child_src = target_parent->children + left_index + KEY_REMOVE_OFFSET;
        size = sizeof(tree_node*) *
               target_parent->current_size - left_index;
        memmove(child_dest,child_src,size);

        target_parent->current_size -= 1;
        target->current_size += left_child->current_size + 1;
        free(left_child->children);
        free(left_child->key);
        free(left_child);
        head->node_size -=1;

        if (target_parent == head->root && target_parent->current_size < 1){
            free(head->root->children);
            free(head->root->key);
            free(head->root);
            head->node_size -= 1;
            head->root = target;
            target->parent == NULL;
            return 0;
        }

        check_node_underflow(target_parent,head);
        return 0;
    }

    if(right_key != NULL && right_child != NULL){
        key_node ** key_dest;
        key_node ** key_src;
        tree_node ** child_dest;
        tree_node ** child_src;
        uint64_t size;

        size = right_child->current_size + CHILD_SIZE_OFFSET;
        for (int i = 0; i < size; ++i) {
            tree_node * child_ptr = *(right_child->children + i);
            if (child_ptr != NULL){
                child_ptr->parent = target;
            }
        }

        /*
         * Copy keys and children from the right node
         */
        size = sizeof(key_node*) * (right_child->current_size);
        memcpy(target->key+target->current_size + KEY_REMOVE_OFFSET,
               right_child->key,size);
        size = sizeof(tree_node*) *
               (right_child->current_size + KEY_REMOVE_OFFSET);
        memcpy(target->children+target->current_size + 1,
               right_child->children,size);
        *(target->key+target->current_size) = right_key;
        /*
         * Push key/child in parent forward
         */
        key_dest = target_parent->key + right_index;
        key_src = target_parent->key + right_index + KEY_REMOVE_OFFSET;
        size = sizeof(key_node*) *
               target_parent->current_size - right_index - KEY_REMOVE_OFFSET;
        memmove(key_dest,key_src,size);

        child_dest = target_parent->children + right_index + KEY_REMOVE_OFFSET;
        child_src = target_parent->children + right_index + PUSH_OFFSET;
        size = sizeof(tree_node*) *
               target_parent->current_size - right_index - KEY_REMOVE_OFFSET;
        memmove(child_dest,child_src,size);

        target_parent->current_size -= 1;
        target->current_size += right_child->current_size + 1;
        free(right_child->children);
        free(right_child->key);
        free(right_child);
        head->node_size -=1;

        if (target_parent == head->root && target_parent->current_size < 1){
            free(head->root->children);
            free(head->root->key);
            free(head->root);
            head->node_size -= 1;
            head->root = target;
            target->parent == NULL;
            return 0;
        }

        check_node_underflow(target_parent,head);
        return 0;
    }
    return 1;
}

int64_t divide_data(uint8_t ** data, uint64_t count){
    if(count % 8 == 0){
        return count / 8;
    }else{
        uint64_t padding = 8 - (count % 8);
        *data = realloc(*data,count + padding);

        if (*data == NULL){
            return -1;
        }

        memset(*data + count, 0 , padding);
        return (count + padding) / 8;
    }
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
    tree->minimum = ceil((branching-1) / 2);
    pthread_rwlock_init(&tree->lock,NULL);
    return tree;
}



void close_store(void * helper) {
    header * head = helper;


    dfs_free(head->root);
    pthread_rwlock_destroy(&head->lock);
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

    //Allocating space for the new key and data
    key_node * new_key = (key_node *)malloc(sizeof(key_node));
    void * data = malloc(count);

    if (new_key == NULL || data == NULL){
        return 1;
    }

    memset(new_key,0,sizeof(key_node));
    memset(data,0,count);
    memcpy(data,plaintext,count);
    int64_t chunk_size = divide_data((uint8_t **)&data,count);

    if(chunk_size == -1){
        return 1;
    }

    encrypt_tea_ctr(data,encryption_key,nonce,data,chunk_size);

    new_key->key_val = key;
    new_key->size = count;
    new_key->data = data;
    new_key->nonce = nonce;
    new_key->chunk_size = chunk_size;

    encrypt_key_cpy(new_key->key, encryption_key);


    // Check if the key already exists in the tree.
    struct info check;
    if(btree_retrieve(key,&check,helper) == 0){
        free_key(new_key);
        return 1;
    }

    /*
     * rwlock Lock
     */
    pthread_rwlock_wrlock(&head->lock);

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
                pthread_rwlock_unlock(&head->lock);
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
        pthread_rwlock_unlock(&head->lock);
        return 1;
    }

    node_add_key(current_node,new_key,NULL,head);

    head->key_size += 1;
    check_node_overflow(current_node,head);

    pthread_rwlock_unlock(&head->lock);
    return 0;
}

int btree_retrieve(uint32_t key, struct info * found, void * helper) {
    header * head = helper;
    tree_node * current_node = head->root;
    tree_node * next_node = NULL;

    pthread_rwlock_rdlock(&head->lock);

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
                pthread_rwlock_unlock(&head->lock);
                return 0;
            }
        }

        if (current_node->status == LEAF){
            break;
        }

        current_node = next_node;
        next_node = NULL;
    }
    pthread_rwlock_unlock(&head->lock);
    return 1;
}

int btree_decrypt(uint32_t key, void * output, void * helper) {
    header * head = helper;
    tree_node * current_node = head->root;
    tree_node * next_node = NULL;

    pthread_rwlock_rdlock(&head->lock);

    while (current_node != NULL){
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



                void * decrypt_tmp = malloc(key_ptr->chunk_size * BITS_BYTE);
                void * data_tmp = malloc(key_ptr->chunk_size * BITS_BYTE);
                if(decrypt_tmp == NULL){
                    pthread_rwlock_unlock(&head->lock);
                    return 1;
                }
                if(data_tmp == NULL){
                    free(decrypt_tmp);
                    pthread_rwlock_unlock(&head->lock);
                    return 1;
                }
                uint32_t key_tmp[4];
                uint64_t nonce = key_ptr->nonce;
                uint32_t size = key_ptr->size;
                int64_t chunk_size = key_ptr->chunk_size;

                memcpy(data_tmp,key_ptr->data,chunk_size *8);
                encrypt_key_cpy(key_tmp,key_ptr->key);

                pthread_rwlock_unlock(&head->lock);

                decrypt_tea_ctr(data_tmp,key_tmp,nonce,
                                decrypt_tmp,chunk_size);

                memcpy(output,decrypt_tmp,size);

                free(data_tmp);
                free(decrypt_tmp);
                return 0;
            }
        }

        if (current_node->status == LEAF){
            break;
        }

        current_node = next_node;
        next_node = NULL;
    }
    pthread_rwlock_unlock(&head->lock);
    return 1;
}

int btree_delete(uint32_t key, void * helper) {
    // Your code here
    // Check if the key already exists in the tree.
    header * head = helper;
    tree_node * current_node = head->root;
    tree_node * next_node = NULL;
    uint8_t found = FALSE;

    struct info check;
    if(btree_retrieve(key,&check,helper) == 1){
        return 1;
    }

    pthread_rwlock_wrlock(&head->lock);

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
                found = TRUE;
                break;
            }
        }

        if (found == TRUE || current_node->status == LEAF){
            break;
        }

        current_node = next_node;
        next_node = NULL;
    }

    if (found == FALSE){
        pthread_rwlock_unlock(&head->lock);
        return 1;
    }
    tree_node * target;
    target = swap_and_remove(current_node,key,head);

    if (target == NULL){
        pthread_rwlock_unlock(&head->lock);
        return 1;
    }
    if (target->current_size >= head->minimum){
        pthread_rwlock_unlock(&head->lock);
        return 0;
    }
    check_node_underflow(target,head);
    pthread_rwlock_unlock(&head->lock);
    return 0;
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

    pthread_rwlock_rdlock(&head->lock);
    dfs_export(head->root,list,counter);
    pthread_rwlock_unlock(&head->lock);

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
    uint64_t tmp3;

    for (int i = 0; i < num_blocks; i++){
        tmp1 = i ^ nonce;
        tmp3 = plain[i];
        encrypt_tea((uint32_t*)&tmp1,(uint32_t*)&tmp2,key);
        cipher[i] = tmp3 ^ tmp2;
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