#include "../src/vm.h"
#include "../src/memory.h"

#include "ubench.h"
#include <assert.h>

UBENCH_EX(Bench, Zoo) {
    VM vm;
    (void) vm;
    InterpretResult ires;
    const char src[] = "class Zoo { \n"
                       "  init() { \n"
                       "    this.aardvark = 1; \n"
                       "    this.baboon = 1; \n"
                       "    this.cat = 1; \n"
                       "    this.donkey = 1; \n"
                       "    this.elephant = 1; \n"
                       "    this.fox = 1; \n"
                       "  } \n"
                       "  ant() { return this.aardvark; } \n"
                       "  banana() { return this.baboon; } \n"
                       "  tuna() { return this.cat; } \n"
                       "  hay() { return this.donkey; } \n"
                       "  grass() { return this.elephant; } \n"
                       "  mouse() { return this.fox; } \n"
                       "} \n"
                       "var zoo = Zoo(); \n"
                       "var sum = 0; \n"
                       "while (sum < 6000000) { \n"
                       "  sum = sum + \n"
                       "      zoo.ant() + \n"
                       "      zoo.banana() + \n"
                       "      zoo.tuna() + \n"
                       "      zoo.hay() + \n"
                       "      zoo.grass() + \n"
                       "      zoo.mouse(); \n"
                       "} \n";

    initVM(stdout, stderr);
    UBENCH_DO_BENCHMARK() { ires = interpret(src); }
    assert(ires == INTERPRET_OK);
    freeVM();
}

UBENCH_MAIN();
