#include "../src/common.h"
#include "../src/memory.h"
#include "../src/vm.h"
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

VMCase assignment[] = {
    {INTERPRET_OK,
     "var a=\"a\"; var b =\"b\"; var c =\"c\"; a = b = c; print a; print b; "
     "print c;",
     "c\nc\nc\n"},
    {INTERPRET_OK,
     "var a =\"before\"; print a; a = \"after\"; print a; print a = \"arg\"; "
     "print a;",
     "before\nafter\narg\narg\n"},
    {INTERPRET_COMPILE_ERROR, "var a = \"a\"; (a) = \"value\";",
     "[line 1] Error at '=': Invalid assignment target.\n"},
    {INTERPRET_COMPILE_ERROR, "var a=\"a\"; var b =\"b\"; a + b = \"value\";",
     "[line 1] Error at '=': Invalid assignment target.\n"},
    {INTERPRET_OK,
     "{\n"
     "  var a = \"before\";\n"
     "  print a; // expect: before\n"
     "\n"
     "  a = \"after\";\n"
     "  print a; // expect: after\n"
     "\n"
     "  print a = \"arg\"; // expect: arg\n"
     "  print a; // expect: arg\n"
     "}\n",

     "before\nafter\narg\narg\n"},
    {INTERPRET_COMPILE_ERROR,
     "var a = \"a\";\n"
     "!a = \"value\"; // Error at '=': Invalid assignment target.\n",
     "[line 2] Error at '=': Invalid assignment target.\n"},
    {INTERPRET_OK,

     "// Assignment on RHS of variable.\n"
     "var a = \"before\";\n"
     "var c = a = \"var\";\n"
     "print a; // expect: var\n"
     "print c; // expect: var\n",
     "var\nvar\n"},
    {INTERPRET_COMPILE_ERROR,
     "class Foo {\n"
     "  Foo() {\n"
     "    this = \"value\"; // Error at '=': Invalid assignment target.\n"
     "  }\n"
     "}\n"
     "\n"
     "Foo();\n",
     "[line 3] Error at '=': Invalid assignment target.\n"},
    {INTERPRET_RUNTIME_ERROR,
     "unknown = \"what\"; // expect runtime error: Undefined variable "
     "'unknown'.\n",
     "Undefined variable 'unknown'.\n[line 1] in script\n"},

};
VM_TEST(Assignment, assignment, 9)

VMCase block[] = {
    {INTERPRET_OK,
     "{} // By itself.\n"
     "\n"
     "// In a statement.\n"
     "if (true) {}\n"
     "if (false) {} else {}\n"
     "\n"
     "print \"ok\"; // expect: ok\n",
     "ok\n"},
    {INTERPRET_OK,

     "var a = \"outer\";\n"
     "\n"
     "{\n"
     "  var a = \"inner\";\n"
     "  print a; // expect: inner\n"
     "}\n"
     "\n"
     "print a; // expect: outer\n",
     "inner\nouter\n"},
};
VM_TEST(Block, block, 2)

VMCase _bool[] = {
    {INTERPRET_OK,
     "print true == true;    // expect: true\n"
     "print true == false;   // expect: false\n"
     "print false == true;   // expect: false\n"
     "print false == false;  // expect: true\n"
     "\n"
     "// Not equal to other types.\n"
     "print true == 1;        // expect: false\n"
     "print false == 0;       // expect: false\n"
     "print true == \"true\";   // expect: false\n"
     "print false == \"false\"; // expect: false\n"
     "print false == \"\";      // expect: false\n"
     "\n"
     "print true != true;    // expect: false\n"
     "print true != false;   // expect: true\n"
     "print false != true;   // expect: true\n"
     "print false != false;  // expect: false\n"
     "\n"
     "// Not equal to other types.\n"
     "print true != 1;        // expect: true\n"
     "print false != 0;       // expect: true\n"
     "print true != \"true\";   // expect: true\n"
     "print false != \"false\"; // expect: true\n"
     "print false != \"\";      // expect: true\n",

     "true\nfalse\nfalse\ntrue\n"
     "false\nfalse\nfalse\nfalse\nfalse\n"
     "false\ntrue\ntrue\nfalse\n"
     "true\ntrue\ntrue\ntrue\ntrue\n"},
    {INTERPRET_OK,
     "print !true;    // expect: false\n"
     "print !false;   // expect: true\n"
     "print !!true;   // expect: true\n",

     "false\ntrue\ntrue\n"},

};
VM_TEST(Bool, _bool, 2)

VMCase call[] = {
    {INTERPRET_RUNTIME_ERROR, "true();",
     "Can only call functions and classes.\n[line 1] in script\n"},

    {INTERPRET_RUNTIME_ERROR, "nil();",
     "Can only call functions and classes.\n[line 1] in script\n"},

    {INTERPRET_RUNTIME_ERROR, "123();",
     "Can only call functions and classes.\n[line 1] in script\n"},
    {INTERPRET_RUNTIME_ERROR,
     "class Foo {}\n"
     "\n"
     "var foo = Foo();\n"
     "foo(); // expect runtime error: Can only call functions and classes.\n",
     "Can only call functions and classes.\n[line 4] in script\n"},

    {INTERPRET_RUNTIME_ERROR, "\"str\"();",
     "Can only call functions and classes.\n[line 1] in script\n"},

};
VM_TEST(Call, call, 5)

VMCase classes[] = {
    {INTERPRET_OK, "class Foo{}\n\nprint Foo;", "Foo\n"},
    {INTERPRET_COMPILE_ERROR, "class Foo < Foo {}",
     "[line 1] Error at 'Foo': A class can't inherit from itself.\n"},
    {INTERPRET_OK,
     "class Foo {\n"
     "  inFoo() {\n"
     "    print \"in foo\";\n"
     "  }\n"
     "}\n"
     "\n"
     "class Bar < Foo {\n"
     "  inBar() {\n"
     "    print \"in bar\";\n"
     "  }\n"
     "}\n"
     "\n"
     "class Baz < Bar {\n"
     "  inBaz() {\n"
     "    print \"in baz\";\n"
     "  }\n"
     "}\n"
     "\n"
     "var baz = Baz();\n"
     "baz.inFoo(); // expect: in foo\n"
     "baz.inBar(); // expect: in bar\n"
     "baz.inBaz(); // expect: in baz\n",

     "in foo\nin bar\nin baz\n"},
    {INTERPRET_OK,
     "class A {}\n"
     "\n"
     "fun f() {\n"
     "  class B < A {}\n"
     "  return B;\n"
     "}\n"
     "\n"
     "print f(); // expect: B\n",
     "B\n"},
    {INTERPRET_COMPILE_ERROR,
     "{\n"
     "  class Foo < Foo {} // Error at 'Foo': A class can't inherit from "
     "itself.\n"
     "}\n"
     "// [c line 5] Error at end: Expect '}' after block.\n",
     "[line 2] Error at 'Foo': A class can't inherit from itself.\n"
     "[line 5] Error at end: Expect '}' after block.\n"},
    {INTERPRET_OK,
     "{\n"
     "  class Foo {\n"
     "    returnSelf() {\n"
     "      return Foo;\n"
     "    }\n"
     "  }\n"
     "\n"
     "  print Foo().returnSelf(); // expect: Foo\n"
     "}\n",
     "Foo\n"},
    {INTERPRET_OK,
     "  class Foo {\n"
     "    returnSelf() {\n"
     "      return Foo;\n"
     "    }\n"
     "  }\n"
     "\n"
     "  print Foo().returnSelf(); // expect: Foo\n",

     "Foo\n"}};
VM_TEST(Class, classes, 7)

VMCase closure[] = {
    {INTERPRET_OK,
     "var f;\n"
     "var g;\n"
     "\n"
     "{\n"
     "  var local = \"local\";\n"
     "  fun f_() {\n"
     "    print local;\n"
     "    local = \"after f\";\n"
     "    print local;\n"
     "  }\n"
     "  f = f_;\n"
     "\n"
     "  fun g_() {\n"
     "    print local;\n"
     "    local = \"after g\";\n"
     "    print local;\n"
     "  }\n"
     "  g = g_;\n"
     "}\n"
     "\n"
     "f();\n"
     "// expect: local\n"
     "// expect: after f\n"
     "\n"
     "g();\n"
     "// expect: after f\n"
     "// expect: after g\n",
     "local\nafter f\nafter f\nafter g\n"},
    {INTERPRET_OK,
     "var a = \"global\";\n"
     "\n"
     "{\n"
     "  fun assign() {\n"
     "    a = \"assigned\";\n"
     "  }\n"
     "\n"
     "  var a = \"inner\";\n"
     "  assign();\n"
     "  print a; // expect: inner\n"
     "}\n"
     "\n"
     "print a; // expect: assigned\n",

     "inner\nassigned\n"},
    {INTERPRET_OK,
     "var f;\n"
     "\n"
     "fun foo(param) {\n"
     "  fun f_() {\n"
     "    print param;\n"
     "  }\n"
     "  f = f_;\n"
     "}\n"
     "foo(\"param\");\n"
     "\n"
     "f(); // expect: param\n",
     "param\n"},
    {INTERPRET_OK,
     "fun f() {\n"
     "  var a = \"a\";\n"
     "  var b = \"b\";\n"
     "  fun g() {\n"
     "    print b; // expect: b\n"
     "    print a; // expect: a\n"
     "  }\n"
     "  g();\n"
     "}\n"
     "f();\n",

     "b\na\n"},
    {INTERPRET_OK,
     "var f;\n"
     "\n"
     "class Foo {\n"
     "  method(param) {\n"
     "    fun f_() {\n"
     "      print param;\n"
     "    }\n"
     "    f = f_;\n"
     "  }\n"
     "}\n"
     "\n"
     "Foo().method(\"param\");\n"
     "f(); // expect: param\n",

     "param\n"},
    {INTERPRET_OK,
     "var f;\n"
     "\n"
     "{\n"
     "  var local = \"local\";\n"
     "  fun f_() {\n"
     "    print local;\n"
     "  }\n"
     "  f = f_;\n"
     "}\n"
     "\n"
     "f(); // expect: local\n",

     "local\n"},
    {INTERPRET_OK,
     "var f;\n"
     "\n"
     "fun f1() {\n"
     "  var a = \"a\";\n"
     "  fun f2() {\n"
     "    var b = \"b\";\n"
     "    fun f3() {\n"
     "      var c = \"c\";\n"
     "      fun f4() {\n"
     "        print a;\n"
     "        print b;\n"
     "        print c;\n"
     "      }\n"
     "      f = f4;\n"
     "    }\n"
     "    f3();\n"
     "  }\n"
     "  f2();\n"
     "}\n"
     "f1();\n"
     "\n"
     "f();\n"
     "// expect: a\n"
     "// expect: b\n"
     "// expect: c\n",

     "a\nb\nc\n"},
    {INTERPRET_OK,
     "{\n"
     "  var local = \"local\";\n"
     "  fun f() {\n"
     "    print local; // expect: local\n"
     "  }\n"
     "  f();\n"
     "}\n",

     "local\n"},
    {INTERPRET_OK,
     "var f;\n"
     "\n"
     "{\n"
     "  var a = \"a\";\n"
     "  fun f_() {\n"
     "    print a;\n"
     "    print a;\n"
     "  }\n"
     "  f = f_;\n"
     "}\n"
     "\n"
     "f();\n"
     "// expect: a\n"
     "// expect: a\n",

     "a\na\n"},
    {INTERPRET_OK,
     "{\n"
     "  var f;\n"
     "\n"
     "  {\n"
     "    var a = \"a\";\n"
     "    fun f_() { print a; }\n"
     "    f = f_;\n"
     "  }\n"
     "\n"
     "  {\n"
     "    // Since a is out of scope, the local slot will be reused by b. Make "
     "sure\n"
     "    // that f still closes over a.\n"
     "    var b = \"b\";\n"
     "    f(); // expect: a\n"
     "  }\n"
     "}\n",

     "a\n"},
    {INTERPRET_OK,
     "{\n"
     "  var foo = \"closure\";\n"
     "  fun f() {\n"
     "    {\n"
     "      print foo; // expect: closure\n"
     "      var foo = \"shadow\";\n"
     "      print foo; // expect: shadow\n"
     "    }\n"
     "    print foo; // expect: closure\n"
     "  }\n"
     "  f();\n"
     "}\n",

     "closure\nshadow\nclosure\n"},
    {INTERPRET_OK,
     "// This is a regression test. There was a bug where the VM would try to "
     "close\n"
     "// an upvalue even if the upvalue was never created because the codepath "
     "for\n"
     "// the closure was not executed.\n"
     "\n"
     "{\n"
     "  var a = \"a\";\n"
     "  if (false) {\n"
     "    fun foo() { a; }\n"
     "  }\n"
     "}\n"
     "\n"
     "// If we get here, we didn't segfault when a went out of scope.\n"
     "print \"ok\"; // expect: ok\n",

     "ok\n"},
    {INTERPRET_OK,
     "// This is a regression test. When closing upvalues for discarded "
     "locals, it\n"
     "// wouldn't make sure it discarded the upvalue for the correct stack "
     "slot.\n"
     "//\n"
     "// Here we create two locals that can be closed over, but only the first "
     "one\n"
     "// actually is. When \"b\" goes out of scope, we need to make sure we "
     "don't\n"
     "// prematurely close \"a\".\n"
     "var closure;\n"
     "\n"
     "{\n"
     "  var a = \"a\";\n"
     "\n"
     "  {\n"
     "    var b = \"b\";\n"
     "    fun returnA() {\n"
     "      return a;\n"
     "    }\n"
     "\n"
     "    closure = returnA;\n"
     "\n"
     "    if (false) {\n"
     "      fun returnB() {\n"
     "        return b;\n"
     "      }\n"
     "    }\n"
     "  }\n"
     "\n"
     "  print closure(); // expect: a\n"
     "}\n",

     "a\n"},
};
VM_TEST(Closure, closure, 13)

VMCase comments[] = {
    {INTERPRET_OK, "print \"ok\";\n// comment", "ok\n"},
    {INTERPRET_OK, "// comment", ""},
    {INTERPRET_OK,
     "// Unicode characters are allowed in comments.\n"
     "//\n"
     "// Latin 1 Supplement: £§¶ÜÞ\n"
     "// Latin Extended-A: ĐĦŋœ\n"
     "// Latin Extended-B: ƂƢƩǁ\n"
     "// Other stuff: ឃᢆ᯽₪ℜ↩⊗┺░\n"
     "// Emoji: ☃☺♣\n"
     "\n"
     "print \"ok\"; // expect: ok\n",

     "ok\n"},

};
VM_TEST(Comments, comments, 3)

VMCase constructor[] = {
    {INTERPRET_OK,
     "class Foo {\n"
     "  init(a, b) {\n"
     "    print \"init\"; // expect: init\n"
     "    this.a = a;\n"
     "    this.b = b;\n"
     "  }\n"
     "}\n"
     "\n"
     "var foo = Foo(1, 2);\n"
     "print foo.a; // expect: 1\n"
     "print foo.b; // expect: 2\n",

     "init\n1\n2\n"},
    {INTERPRET_OK,
     "class Foo {\n"
     "  init() {\n"
     "    print \"init\";\n"
     "    return;\n"
     "    print \"nope\";\n"
     "  }\n"
     "}\n"
     "\n"
     "var foo = Foo(); // expect: init\n"
     "print foo.init(); // expect: init\n"
     "// expect: Foo instance\n",

     "init\ninit\nFoo instance\n"},
    {INTERPRET_OK,
     "class Foo {\n"
     "  init(arg) {\n"
     "    print \"Foo.init(\" + arg + \")\";\n"
     "    this.field = \"init\";\n"
     "  }\n"
     "}\n"
     "\n"
     "var foo = Foo(\"one\"); // expect: Foo.init(one)\n"
     "foo.field = \"field\";\n"
     "\n"
     "var foo2 = foo.init(\"two\"); // expect: Foo.init(two)\n"
     "print foo2; // expect: Foo instance\n"
     "\n"
     "// Make sure init() doesn't create a fresh instance.\n"
     "print foo.field; // expect: init\n",

     "Foo.init(one)\nFoo.init(two)\nFoo instance\ninit\n"},
    {INTERPRET_OK,
     "class Foo {}\n"
     "\n"
     "var foo = Foo();\n"
     "print foo; // expect: Foo instance\n",

     "Foo instance\n"},
    {INTERPRET_RUNTIME_ERROR,
     "class Foo {}\n"
     "\n"
     "var foo = Foo(1, 2, 3); // expect runtime error: Expected 0 arguments "
     "but got 3.\n",

     "Expected 0 arguments but got 3.\n[line 3] in script\n"},
    {INTERPRET_OK,
     "class Foo {\n"
     "  init() {\n"
     "    print \"init\";\n"
     "    return;\n"
     "    print \"nope\";\n"
     "  }\n"
     "}\n"
     "\n"
     "var foo = Foo(); // expect: init\n"
     "print foo; // expect: Foo instance\n",

     "init\nFoo instance\n"},
    {INTERPRET_RUNTIME_ERROR,
     "class Foo {\n"
     "  init(a, b) {\n"
     "    this.a = a;\n"
     "    this.b = b;\n"
     "  }\n"
     "}\n"
     "\n"
     "var foo = Foo(1, 2, 3, 4); // expect runtime error: Expected 2 arguments "
     "but got 4.\n",

     "Expected 2 arguments but got 4.\n[line 8] in script\n"},
    {INTERPRET_OK,
     "class Foo {\n"
     "  init(arg) {\n"
     "    print \"Foo.init(\" + arg + \")\";\n"
     "    this.field = \"init\";\n"
     "  }\n"
     "}\n"
     "\n"
     "fun init() {\n"
     "  print \"not initializer\";\n"
     "}\n"
     "\n"
     "init(); // expect: not initializer\n",

     "not initializer\n"},
    {INTERPRET_RUNTIME_ERROR,
     "class Foo {\n"
     "  init(a, b) {}\n"
     "}\n"
     "\n"
     "var foo = Foo(1); // expect runtime error: Expected 2 arguments but got "
     "1.\n",

     "Expected 2 arguments but got 1.\n[line 5] in script\n"},
    {INTERPRET_OK,
     "class Foo {\n"
     "  init() {\n"
     "    fun init() {\n"
     "      return \"bar\";\n"
     "    }\n"
     "    print init(); // expect: bar\n"
     "  }\n"
     "}\n"
     "\n"
     "print Foo(); // expect: Foo instance\n",

     "bar\nFoo instance\n"},

    {INTERPRET_COMPILE_ERROR,
     "class Foo {\n"
     "  init() {\n"
     "    return \"result\"; // Error at 'return': Can't return a value from "
     "an initializer.\n"
     "  }\n"
     "}\n",

     "[line 3] Error at 'return': Can't return a value from an initializer.\n"},
};
VM_TEST(Constructor, constructor, 11)

VMCase lists[] = {
    {INTERPRET_OK, "print [nil][0];", "nil\n"},
    {INTERPRET_OK, " var l = [1, 2]; print l[1];", "2\n"},
    {INTERPRET_RUNTIME_ERROR, "var l = [1,2]; print l[2];",
     "List index (2) out of bounds (2)\n[line 1] in script\n"},
    {INTERPRET_RUNTIME_ERROR, "var l = [1,2]; print l[-3];",
     "List index (-3) out of bounds (2)\n[line 1] in script\n"},
};
VM_TEST(List, lists, 4)

UTEST_MAIN()
