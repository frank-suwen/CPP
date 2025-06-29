#include <vector>
#include <iostream>

int main() {
    std::vector<int> v{1, 2, 3, 4};

    // EXPLICIT: keep a reference, so mutation affects v
    for (const int& x : v) {    // same effect with 'auto&' below
        std::cout << x << ' ';
    }

    std::cout << '\n';

    // AUTO, but forgetting '&' - makes a **copy** of each element
    for (auto x : v) {         // x is int, not int&
        std::cout << x << ' '; // v won't change even if we modify x here
    }

    std::cout << '\n';
}