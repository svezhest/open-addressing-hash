#pragma once

#include <cstdlib>

class LinearProbing {
    size_t ind;

public:
    LinearProbing() {
        start();
    }

    void start() {
        ind = 0;
    }

    size_t next() {
        return ++ind;
    }
};

class QuadraticProbing {
    size_t ind;

public:
    QuadraticProbing() {
        start();
    }

    void start() {
        ind = 0;
    }

    size_t next() {
        ++ind;
        return ind * ind;
    }
};
