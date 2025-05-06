#include "blockchain.h"
#include <iostream>

using namespace std;

Blockchain::Blockchain() {
    vector<Transaction> genesisTxs = {
        {"Network", "Alice", 50},
        {"Network", "Bob", 50}
    };
    Block genesisBlock(string(64, '0'), genesisTxs);
    genesisBlock.nonce = 0;
    genesisBlock.timestamp = 0;
    chain[genesisBlock.hash] = genesisBlock;
    blockOrder.push_back(genesisBlock.hash);
    tipHash = genesisBlock.hash;
    cout << "Genesis block created with hash: " << tipHash << endl;
}

void Blockchain::addBlock(const Block& block) {
    chain[block.hash] = block;
    blockOrder.push_back(block.hash);
    tipHash = block.hash;
    displayBlock(block);
}


void Blockchain::displayBlock(const Block& block) const {
    cout << "------ New Block ------\n";
    cout << "Parent Hash: " << block.parentHash << endl;
    cout << "Nonce: " << block.nonce << endl;
    cout << "Difficulty: " << block.difficulty << endl;
    cout << "Timestamp: " << block.timestamp << endl;
    cout << "Merkle Root: " << block.merkleRoot << endl;
    cout << "Hash: " << block.hash << endl;
    cout << "Current Blockchain Height: " << chain.size() - 1 << endl;
    cout << "-----------------------------\n";
}

void Blockchain::displayBlockchainHashes() const {
    cout << "\nBlockchain from Genesis to Tip:\n";
    for (const auto& hash : blockOrder) {
        cout << "Block Hash: " << hash << endl;
    }

    cout << "\nBlockchain from Tip to Genesis:\n";
    string currentHash = tipHash;
    while (currentHash != string(64, '0')) {
        cout << "Block Hash: " << currentHash << endl;
        currentHash = chain.at(currentHash).parentHash;
    }
}

string Blockchain::getTip() const {
    return tipHash;
}
