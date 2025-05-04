#include "utest.h"

#include "../src/vm.h"

UTEST(Test, List) {
    VM vm;
    (void) vm;
    InterpretResult ires;

    const char src[] = "print [nil][0];";

    initVM();
    ires = interpret(src);
    EXPECT_EQ(ires, INTERPRET_OK);
    freeVM();
}

UTEST(Test, ListVar) {
    VM vm;
    (void) vm;
    InterpretResult ires;

    const char src[] = " var l = [1, 2]; print l[1];";

    initVM();
    ires = interpret(src);
    EXPECT_EQ(ires, INTERPRET_OK);
    freeVM();
}


UTEST_MAIN()
