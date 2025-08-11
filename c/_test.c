#define main my_main
#include "hello_world.c"
#undef main
/***************************************************************/
#include "ut/ut.h"
void n_hello(int n) {
    int i = 0;
    while (i < n) {
        hello();
        i++;
    }
}
TESTMETHOD(test4) {
    hello_count = 0;
    n_hello(6);
    ASSERT_EQ(6, hello_count);
}
TESTMETHOD(test3) {
    hello_count = 0;
    int i = 0;
    while (i < 5) {
        hello();
        i++;
    }
    ASSERT_EQ(5, hello_count);
}
TESTMETHOD(test2) {
    hello_count = 0;
    for (int i = 0; i < 5; i++) {
        hello();
    }
    ASSERT_EQ(5, hello_count);
}
TESTMETHOD(test1) {
    my_main(0,0);
//    ASSERT(0);
}
