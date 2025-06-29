#include <tuple>
#include <string>
#include <vector>

int main() {
    std::vector<std::tuple<int, std::string>> data;

    // function returning an iterator to a tuple
    auto find_id = [&](int id) {
        return std::find_if(data.begin(), data.end(),
                            [&](auto& t){ return std::get<0>(t) == id; });
    };

    std::vector<std::tuple<int, std::string>>::iterator it_explicit = 
        std::find_if(data.begin(), data.end(),
                    [&](std::tuple<int, std::string>& t){ return std::get<0>(t) == 42; });
    
    // same with auto
    auto it_auto = find_id(42);
}