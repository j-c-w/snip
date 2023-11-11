// RUN: clang -cc1 -load %shlibdir/libHelloWorld%shlibext -plugin hello-world %s 2>&1 | FileCheck %s
#include<stdio.h>

int k = 0;

struct Bar {
  int j;
};

union Bez {
  int k;
  float l;
};
int g(int x) {
	return x;
}

void f() {
	printf("Hellworlld");
	int x = g(4);
	k += x;
	union Bez k;
}

// CHECK: (clang-tutor) file: {{.*}}/clang-tutor/test/HelloWorld-basic.cpp
// CHECK-NEXT: (clang-tutor)  count: 3
