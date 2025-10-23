#pragma once

#include <cstdlib>

class Random {

public:
    // Procedural interface
    static void srand(unsigned seed) { s_instance.o_srand(seed); }
    static unsigned rand() { return s_instance.o_rand(); }

    // N.B. randint returns a number strictly less than max,
    // unlike std::experimental::randint.
    // randint is also biased.  It's cheaper than an unbiased version.
    template <class MinType, class MaxType>
    static MaxType randint(MinType min, MaxType max) {
        assert(0 <= min);
        assert(max <= RAND_MAX);
        return (MaxType)s_instance.o_randint(min, max); 
    }

private:
    Random();
    Random(const Random&) = delete;
    void operator = (const Random&) = delete;
    ~Random() {}

    // Object interface is private.
    // We do this so we can privately use a noise source as
    // the default seed.
    void o_srand(unsigned seed) { std::srand(seed); }
    unsigned o_rand();
    unsigned o_randint(unsigned min, unsigned max) {
        return min + o_rand() % (max - min);
    }
 
    static Random s_instance;
};
