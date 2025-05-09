#include "../src/vm.h"
#include "../src/memory.h"
#include "utest.h"
#include <stdio.h>

typedef struct {
    InterpretResult ires;
    const char *code;
    const char *expected;
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
    FileStream fout, ferr;
    initFileStream(&fout);
    initFileStream(&ferr);

    initVM(fout.fp, ferr.fp);
    InterpretResult result = interpret(testCase->code);
    fflush(fout.fp);
    fflush(ferr.fp);

    EXPECT_EQ(result, testCase->ires);
    if (testCase->ires == INTERPRET_OK) {
        EXPECT_STREQ(testCase->expected, fout.buf);
        EXPECT_STREQ("", ferr.buf);
    } else {
        EXPECT_STREQ(testCase->expected, ferr.buf);
    }
    freeFileStream(&fout);
    freeFileStream(&ferr);
    freeVM();
}

VMCase classes[] = {
    {INTERPRET_OK, "class F{} var f = F(); f.x = 1; print f.x;", "1\n"},
    {INTERPRET_OK, "class Foo{ returnSelf() { return Foo;}}", ""},
};
VM_TEST(Class, classes, 2)

VMCase lists[] = {
    {INTERPRET_OK, "print [nil][0];", "nil\n"},
    {INTERPRET_OK, " var l = [1, 2]; print l[1];", "2\n"},
};
VM_TEST(List, lists, 2)


UTEST_MAIN()
