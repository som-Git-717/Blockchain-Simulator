#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <unordered_map>
#include <vector>
#include "block.h"

using namespace std;

class Blockchain {
private:
    unordered_map<string, Block> chain;
    vector<string> blockOrder;
    string tipHash;

public:
    Blockchain();
    void addBlock(const Block& block);
    void displayBlock(const Block& block) const;
    void displayBlockchainHashes() const;
    string getTip() const;
    string getTipHash() const { return tipHash; }
};

#endif
