#include<windows.h>
#include "tests.h"

int main(void) {
    system("color f0");
    SetConsoleOutputCP(65001);
    run_automatic_tests();
    run_demo();
    return 0;
}