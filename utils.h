//
// Created by Manik mehta on 10/06/26.
//

#ifndef RAFT_CPP_UTILS_H
#define RAFT_CPP_UTILS_H

#include <random>
#include <mutex>

class ThreadSafeRandom {
    std::mt19937_64 gen_;
    std::mutex mu_;
public:
    int Intn(int n);
};

#endif //RAFT_CPP_UTILS_H
