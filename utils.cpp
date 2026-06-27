//
// Created by Manik mehta on 10/06/26.
//

#include "utils.h"

int ThreadSafeRandom::Intn(const int n) {
    std::lock_guard lock(mu_);
    std::uniform_int_distribution distribution(0, n);
    return distribution(gen_);
}
