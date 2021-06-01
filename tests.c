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

    void * helper = init_store(4, 4);

    char * data = "hello";
    uint32_t enc_key[4] = {0};
    btree_insert(9,data,strlen(data),enc_key,0,helper);
    btree_insert(10,data,strlen(data),enc_key,100,helper);
    btree_insert(12,data,strlen(data),enc_key,100,helper);
    btree_insert(3,data,strlen(data),enc_key,100,helper);
    btree_insert(7,data,strlen(data),enc_key,100,helper);
    btree_insert(45,data,strlen(data),enc_key,100,helper);
    list = NULL;
    num = btree_export(helper, &list);
    print_tree(list,num);

    btree_delete(3,helper);
    debug(helper);
    btree_delete(7,helper);




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
