#include <iostream>

template <typename T> requires { requires true; }

void foo() {
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    std::cout << __cplusplus << std::endl;


    return 0;
}
