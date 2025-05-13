#include <iostream>
#include <memory>

void uniquePtrExample() {
    std::unique_ptr<int> ptr = std::make_unique<int>(10);
    std::cout << "unique_ptr value: " << *ptr << std::endl;

    // Transfer ownership
    std::unique_ptr<int> newPtr = std::move(ptr);
    if (!ptr) {
        std::cout << "Original pointer is now null after move.\n";
    }
}

int main() {
    uniquePtrExample();
    return 0;
}