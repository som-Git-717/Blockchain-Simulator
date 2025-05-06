#include "block.h"
#include "sha256.h"
#include <sstream>
#include <iomanip>

using namespace std;

Block::Block() : nonce(0), timestamp(0) {}

Block::Block(const string& prevHash, const vector<Transaction>& txs) {
    parentHash = prevHash;
    nonce = 1000 + rand() % 9000;
    difficulty = string(1, '0');
    timestamp = time(nullptr);
    transactions = txs;
    merkleRoot = calculateMerkleRoot(transactions);
    hash = calculateHash();
}

Block::Block(const string& prevHash, const vector<Transaction>& txs, const string& validator) {
    parentHash = prevHash;
    nonce = 0;
    difficulty = "PoA";
    timestamp = time(nullptr);
    transactions = txs;
    validatorPublicKey = validator;
    merkleRoot = calculateMerkleRoot(transactions);
    hash = calculateHash();
}

string Block::calculateHash() const {
    stringstream ss;
    if (difficulty == "PoA") {
        ss << parentHash << timestamp << merkleRoot << validatorPublicKey;
    } else {
        ss << parentHash << nonce << difficulty << timestamp << merkleRoot;
    }
    return sha256(ss.str());
}

string Block::calculateMerkleRoot(const vector<Transaction>& transactions) {
    if (transactions.empty())
        return "";

    string combinedHash;
    for (const auto& tx : transactions) {
        stringstream txStream;
        txStream << tx.sender << tx.receiver << tx.amount;
        combinedHash += sha256(txStream.str());
    }
    return sha256(combinedHash);
}