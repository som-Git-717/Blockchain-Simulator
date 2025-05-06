#ifndef POA_CONSENSUS_H
#define POA_CONSENSUS_H

#include "blockchain.h"
#include "transaction.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>

using namespace std;

struct Authority {
    string name;
    string publicKey;
    bool isActive;
    
    Authority() : name(""), publicKey(""), isActive(false) {}
    Authority(const string& n, const string& key)
        : name(n), publicKey(key), isActive(true) {}
};

class PoAConsensus {
private:
    unordered_map<string, Authority> validators;
    mutex mtx;
    size_t currentValidatorIndex;
    vector<string> validatorOrder;

public:
    PoAConsensus(int minerCount=5);
    
    void addValidator(const string& name, const string& publicKey);
    void removeValidator(const string& publicKey);
    bool isAuthorized(const string& publicKey) const;
    string getNextValidator();
    Block createBlock(const string& validatorPublicKey, const string& parentHash, const vector<Transaction>& txs);
    bool validateBlockAuthority(const Block& block, const string& validatorPublicKey, int attack, int validatorIndex) const;
    Block startBlockCreationRound(Blockchain& blockchain, vector<vector<Transaction>>& validatorTxsLists, int attack=0);
    
};

#endif