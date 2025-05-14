#include <iostream>

template<typename T>
class VectorLike {
public:
    VectorLike() : data(new T[2]), capacity(2), length(0) {}

    ~VectorLike() {
        delete[] data;
    }

    void push_back(const T& value) {
        if (length == capacity) {
            resize();
        }
        data[length++] = value;
    }

    T& operator[](size_t index) {
        return data[index];
    }

    size_t size() const {
        return length;
    }

private:
    T* data;
    size_t capacity;
    size_t length;

    void resize() {
        size_t newCapacity = capacity * 2;
        T* newData = new T[newCapacity];

        for (size_t i = 0; i < length; ++i) {
            newData[i] = data[i]; // copy existing elements
        }

        delete[] data; // free old memory
        data = newData;
        capacity = newCapacity;
    }
};

// --- Test Code ---
void vectorLikeExample() {
    VectorLike<int> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    for (size_t i = 0; i < vec.size(); ++i) {
        std::cout << vec[i] << " ";
    }
    std::cout << std:: endl;
}

int main() {
    vectorLikeExample();
    return 0;
}