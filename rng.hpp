#pragma once

#include <stdint.h>

/*
 * Xoshiro512+
 */
class Rng
{
public:
    Rng()
    {
    }

    Rng(uint64_t seed)
    {
        this->seed(seed);
    }

    void seed(uint64_t seed)
    {
        _state[0] = splitmix64(seed);
        _state[1] = splitmix64(_state[0]);
        _state[2] = splitmix64(_state[1]);
        _state[3] = splitmix64(_state[2]);
    }

    uint64_t next()
    {
        const uint64_t result = _state[0] + _state[3];
        const uint64_t t = _state[1] << 17;
        _state[2] ^= _state[0];
        _state[3] ^= _state[1];
        _state[1] ^= _state[2];
        _state[0] ^= _state[3];
        _state[2] ^= t;
        _state[3] = rotl(_state[3], 45);
        return result;
    }

    float fnext()
    {
        return (next() >> 40) * (1.0f / (1ULL << 24));
    }

    double dnext()
    {
        return (next() >> 11) * (1.0 / (1ULL << 53));
    }

private:
    uint64_t _state[4];

    uint64_t rotl(const uint64_t x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    static uint64_t splitmix64(uint64_t seed)
    {
        uint64_t z = (seed += 0x9e3779b97f4a7c15);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
        z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
        return z ^ (z >> 31);
    }
};

