#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#include "btreestore.h"
#include "cmocka.h"
static void test_setup_1(void **state) {
    void * helper = init_store(4, 4);

    char * data = "hello";
    uint32_t enc_key[4] = {0};
    btree_insert(9,data,strlen(data),enc_key,0,helper);
    btree_insert(10,data,strlen(data),enc_key,100,helper);

    struct info * found = calloc(1,sizeof(struct info));
    btree_retrieve(10,found,helper);
    printf("SIZE:%d\n",found->size);
    printf("DATA:%s\n", found->data);
    printf("NONCE:%lu\n",found->nonce);


    free(found);

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
