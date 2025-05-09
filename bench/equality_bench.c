#include "../src/vm.h"
#include "ubench.h"
#include <assert.h>

UBENCH_EX(Bench, Equality) {
    VM vm;
    (void) vm;
    InterpretResult ires;
    const char src[] =
        "var i = 0;"
        " while (i < 10000000) {"
        "   i = i + 1;"
        " "
        "   1; 1; 1; 2; 1; nil; 1; \"str\"; 1; true;"
        "   nil; nil; nil; 1; nil; \"str\"; nil; true;"
        "   true; true; true; 1; true; false; true; \"str\"; true; nil;"
        "   \"str\"; \"str\"; \"str\"; \"stru\"; \"str\"; 1; \"str\"; nil; "
        "\"str\"; true;"
        " }"
        " "
        " "
        " i = 0;"
        " while (i < 10000000) {"
        "   i = i + 1;"
        " "
        "   1 == 1; 1 == 2; 1 == nil; 1 == \"str\"; 1 == true;"
        "   nil == nil; nil == 1; nil == \"str\"; nil == true;"
        "   true == true; true == 1; true == false; true == \"str\"; true == "
        "nil;"
        "   \"str\" == \"str\"; \"str\" == \"stru\"; \"str\" == 1; \"str\" == "
        "nil; \"str\" == true;"
        " }";

    initVM(stdout, stderr);
    UBENCH_DO_BENCHMARK() { ires = interpret(src); }
    assert(ires == INTERPRET_OK);
    freeVM();
}

UBENCH_MAIN();
