#include <iostream>

class MyClass {
public: 
    MyClass() { std::cout << "Default constructor\n"; }
    MyClass(const MyClass&) { std::cout << "Copy constructor\n"; }
    MyClass(MyClass&&) noexcept { std::cout << "Move constructor\n"; }
    ~MyClass() = default;
};

MyClass createRVO() {
    return MyClass(); // RVO happens here - no copy/move
}

int main() {
    std::cout << "RVO Example:\n";
    MyClass obj = createRVO(); //Expect only default constructor if RVO is applied
}