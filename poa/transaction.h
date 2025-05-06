#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

using namespace std;

struct Transaction {
    std::string sender;
    std::string receiver;
    double amount;

    Transaction(std::string s, std::string r, double a)
        : sender(s),receiver(r), amount(a) {}
};

#endif
