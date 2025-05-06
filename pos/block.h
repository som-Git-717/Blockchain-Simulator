#ifndef BLOCK_H
#define BLOCK_H

#include <vector>
#include <string>
#include <ctime>
#include "transaction.h"

using namespace std;

class Block {
public:
    string parentHash, difficulty, merkleRoot, hash;
    int nonce;
    time_t timestamp;
    vector<Transaction> transactions;

    Block();
    Block(const string& prevHash, const vector<Transaction>& txs);

    string calculateHash() const;
    static string calculateMerkleRoot(const vector<Transaction>& transactions);
};

#endif
