#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "btreestore.h"
#include "cmocka.h"

void print_tree(struct node * list, uint64_t num){
    for (int i = 0; i < num; i++){
        int num_of_keys = (list + i)->num_keys;
        printf("NODE: ");
        for (int j = 0; j < num_of_keys; ++j) {
            printf(" %d ", list[i].keys[j]);
        }
        printf("\n");
    }
    printf("\n");
}

static void test_setup_1(void **state) {
    struct node * list = NULL;
    uint64_t num;

    void * helper = init_store(3, 4);

    char * data = "hello";
    uint32_t enc_key[4] = {0};
    btree_insert(2,data,strlen(data),enc_key,0,helper);
    btree_insert(3,data,strlen(data),enc_key,100,helper);
    btree_insert(1,data,strlen(data),enc_key,100,helper);

    btree_insert(8,data,strlen(data),enc_key,100,helper);

    btree_insert(80,data,strlen(data),enc_key,100,helper);

    btree_insert(5,data,strlen(data),enc_key,100,helper);

    btree_insert(6,data,strlen(data),enc_key,100,helper);

    btree_insert(4,data,strlen(data),enc_key,100,helper);

    btree_insert(20,data,strlen(data),enc_key,100,helper);
    debug(helper);
    btree_insert(21,data,strlen(data),enc_key,100,helper);
    debug(helper);
    btree_insert(22,data,strlen(data),enc_key,100,helper);






//    struct info * found = calloc(1,sizeof(struct info));
//    btree_retrieve(10,found,helper);
//
    list = NULL;
    num = btree_export(helper, &list);
    print_tree(list,num);
//
//    printf("SIZE:%d\n",found->size);
//    printf("DATA:%s\n", found->data);
//    printf("NONCE:%lu\n",found->nonce);
//
//
//    free(found);

    close_store(helper);
}
int main() {
    /*
     * Constructing Unit Test
     */
    const struct CMUnitTest tests[] = {
            cmocka_unit_test(test_setup_1),
    };

    /*
     * Run tests
     */
    cmocka_run_group_tests(tests, NULL, NULL);

    return 0;
}
