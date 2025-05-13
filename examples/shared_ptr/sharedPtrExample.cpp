#include <iostream>
#include <memory>

class Demo {
public:
    Demo() {
        std::cout << "Demo constructor\n";    
    }

    ~Demo() {
        std::cout << "Demo destructor\n";
    }

    void sayHello() {
        std::cout << "Hello from Demo\n";
    }
};

void sharedPtrExample() {
    std::shared_ptr<Demo> ptr1 = std::make_shared<Demo>();
    {
        std::shared_ptr<Demo> ptr2 = ptr1; // shared ownership
        std::cout << "Inside block, use_count: " << ptr1.use_count() << std::endl;
        ptr2->sayHello();
    } // ptr2 goes output scope

    std::cout << "Outside block, use_count: " << ptr1.use_count() << std::endl;
}

int main() {
    sharedPtrExample();
    return 0;
}