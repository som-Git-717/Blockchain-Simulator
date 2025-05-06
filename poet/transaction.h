#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

using namespace std;

struct Transaction {
    string sender;
    string receiver;
    double amount;

    Transaction(string s, string r, double a)
        : sender(s), receiver(r), amount(a) {}
};

#endif
