#include "../src/vm.h"
#include "utest.h"

typedef struct {
    InterpretResult ires;
    const char *msg;
} VMCase;

struct VM {
    VMCase *cases;
};

#define VM_TEST(name, data, count)                                             \
    UTEST_I(VM, name, count) {                                                 \
        static_assert(sizeof(data) / sizeof(data[0]) == count, #name);         \
        utest_fixture->cases = data;                                           \
        ASSERT_TRUE(1);                                                        \
    }

UTEST_I_SETUP(VM) {
    (void) utest_index;
    (void) utest_fixture;
    ASSERT_TRUE(1);
}

UTEST_I_TEARDOWN(VM) {
    VMCase *testCase = &utest_fixture->cases[utest_index];
    VM vm;
    (void) vm;
    initVM();
    InterpretResult result = interpret(testCase->msg);
    EXPECT_EQ(result, testCase->ires);
    freeVM();
}

VMCase classes[] = {
    {INTERPRET_OK, "class F{} var f = F(); f.x = 1; print f.x;"},
    {INTERPRET_OK, "class Foo{ returnSelf() { return Foo;}}"},
};
VM_TEST(Class, classes, 2)

VMCase lists[] = {
    {INTERPRET_OK, "print [nil][0];"},
    {INTERPRET_OK, " var l = [1, 2]; print l[1];"},
};
VM_TEST(List, lists, 2)


UTEST_MAIN()
