#include <iostream>

class MyClass {
public:
    MyClass() { std::cout << "Default constructor\n"; }
    MyClass(const MyClass&) { std::cout << "Copy constructor\n"; }
    MyClass(MyClass&&) noexcept { std::cout << "Move constructor\n"; }
    ~MyClass() = default;
};

MyClass createAndMove() {
    MyClass tmp;
    return std::move(tmp);
}

int main() {
    std::cout << "Move Semantics Example:\n";
    MyClass obj = createAndMove(); // Expect default + move constructor
}