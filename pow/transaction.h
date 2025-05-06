#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
using namespace std;

struct Transaction {
    string sender;
    string receiver;
    double amount;

    Transaction(string s, string r, double a)
        :sender(s), receiver(r),amount(a) {}

    bool operator==(const Transaction& other) const {
        return sender == other.sender && receiver == other.receiver && amount == other.amount;
    }
};

#endif
