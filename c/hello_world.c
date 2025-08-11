#include <stdio.h>
int hello_count = 0;
void hello() {
    puts("Hello, world!");
    hello_count++;
}
int main(int argc, char **argv) {
    hello();
    return 0;
}
