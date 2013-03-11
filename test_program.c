#include "stdio.h"

void D() {
    printf("D");
}

void C() {
    printf("C");
}

void B() {
    printf("B");
}

void A() {
    printf("A");
}

void scope1() {
      A(); B(); C(); D();
}
void scope2() {
      A(); C(); D();
}
void scope3() {
      A(); B(); B();
}
void scope4() {
      B(); D(); scope1();
}
void scope5() {
      B(); D(); A();
}
void scope6() {
    B(); D();
}
