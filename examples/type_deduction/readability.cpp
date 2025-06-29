#include <vector>
#include <iostream>

int main() {
    std::vector<int> nums{1, 2, 3, 4};

    // explicit iterator
    for (std::vector<int>::iterator it = nums.begin(); it != nums.end(); ++it) {
        std::cout << *it << ' ';
    }

    std::cout << '\n';

    // same loopo with auto
    for (auto it = nums.begin(); it != nums.end(); ++it) {
        std::cout << *it << ' ';
    }

    std::cout << "\n";
}