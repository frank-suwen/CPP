#include <cassert>
#include "spsc_ring.hpp"

int main() {
    lab::SpscRing<int, 1024> q;
    for (int i = 0; i < 100; ++i) { assert(q.try_push(i)); }
    for (int i = 0; i < 100; ++i) { int v=-1; assert(q.try_pop(v)); assert(v==i); }
    assert(q.approx_size() == 0);
    return 0;
}