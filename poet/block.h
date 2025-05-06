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
    
    // PoET-specific fields
    string waitCertificate;  // In a real implementation, this would be cryptographically verified
    int waitTime;                 // The amount of time this miner waited

    Block();
    Block(const string& prevHash, const vector<Transaction>& txs);
    // Constructor for PoET blocks
    Block(const string& prevHash, const vector<Transaction>& txs, int wait);

    string calculateHash() const;
    static string calculateMerkleRoot(const vector<Transaction>& transactions);
};

#endif