#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>

#include "btreestore.h"
#include "cmocka.h"
static void test_setup_1(void **state) {
    void * helper = init_store(4, 4);


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
