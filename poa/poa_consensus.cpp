#include "poa_consensus.h"
#include "sha256.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>
#include <sstream>
#include <mutex>
#include <atomic>

using namespace std;

atomic<int> verifiedMiners(0);
atomic<long long> totalHashAttempts(0); //global count
atomic<int> totalMiners(0);

PoAConsensus::PoAConsensus(int minerCount) : currentValidatorIndex(0) {
    // Create validators based on minerCount
    for(int i = 0; i < minerCount; i++) {
        string name = "Authority-" + to_string(i+1);
        string key = "auth" + to_string(i+1) + "key" + to_string(123456 + i*111111);
        addValidator(name, key);
    }
}

void PoAConsensus::addValidator(const string& name, const string& publicKey) {
    lock_guard<mutex> lock(mtx);
    validators[publicKey] = Authority(name, publicKey);
    validatorOrder.push_back(publicKey);
}

void PoAConsensus::removeValidator(const string& publicKey) {
    lock_guard<mutex> lock(mtx);
    auto it = validators.find(publicKey);
    if (it != validators.end()) {
        it->second.isActive = false;
        
        auto orderIt = find(validatorOrder.begin(), validatorOrder.end(), publicKey);
        if (orderIt != validatorOrder.end()) {
            validatorOrder.erase(orderIt);
        }
    }
}

bool PoAConsensus::isAuthorized(const string& publicKey) const {
    auto it = validators.find(publicKey);
    return (it != validators.end() && it->second.isActive);
}

string PoAConsensus::getNextValidator() {
    lock_guard<mutex> lock(mtx);
    if (validatorOrder.empty()) {
        return "";
    }
    
    string nextValidator = validatorOrder[currentValidatorIndex];
    currentValidatorIndex = (currentValidatorIndex + 1) % validatorOrder.size();
    return nextValidator;
}

Block PoAConsensus::createBlock(const string& validatorPublicKey, 
                               const string& parentHash, 
                               const vector<Transaction>& txs) {
    if (!isAuthorized(validatorPublicKey)) {
        throw runtime_error("Unauthorized validator");
    }
    
    return Block(parentHash, txs, validatorPublicKey);
}
bool PoAConsensus::validateBlockAuthority(const Block& block, const string& validatorPublicKey, int attack, int validatorIndex) const {
    cout << "[Authority " << validators.at(validatorPublicKey).name << "] verifying the block...\n";

    int attackThreshold = validatorOrder.size() / 2;

    if (attack == 1 && validatorIndex >= attackThreshold) {
        cout << "[Authority " << validators.at(validatorPublicKey).name << "] Block verification ARTIFICIALLY failed! (51\% attack simulation)\n";
        return false;
    }

    if (!isAuthorized(validatorPublicKey)) {
        cout << "[Authority " << validators.at(validatorPublicKey).name << "] Block verification failed: Unauthorized validator.\n";
        return false;
    }

    stringstream ss;
    ss << block.parentHash << block.timestamp << block.merkleRoot << block.validatorPublicKey;
    string expectedHash = sha256(ss.str());

    if (block.hash == expectedHash) {
        cout << "[Authority " << validators.at(validatorPublicKey).name << "] Block verified successfully!\n";
        this_thread::sleep_for(chrono::milliseconds(1000));
        verifiedMiners++;
        totalHashAttempts++;
        return true;
    } else {
        cout << "[Authority " << validators.at(validatorPublicKey).name << "] Block verification failed: Hash mismatch.\n";
        totalHashAttempts++;
        return false;
    }
}


Block PoAConsensus::startBlockCreationRound(Blockchain& blockchain, vector<vector<Transaction>>& validatorTxsLists,int attack) {

    totalHashAttempts = 0;
    verifiedMiners = 0;
    totalMiners = validatorTxsLists.size();

    if (attack == 1) {
        int attackThreshold = totalMiners / 2;
        int attackingMiners = totalMiners - attackThreshold;
        cout << "ATTACK MODE ENABLED: " << attackingMiners << " miners will reject valid blocks\n";
        cout << "Attacking miners: " << attackingMiners << " (" 
                    << (attackingMiners * 100.0 / totalMiners) << "\% of network)\n";
    }

    string currentValidator = getNextValidator();
    if(currentValidator.empty()) {
        throw runtime_error("No active validators available");
    }
    
    const Authority& validator = validators.at(currentValidator);
    cout << validator.name << " selected for this round.\n";
    
    size_t validatorIndex = 0;
    while (validatorIndex < validatorOrder.size() && validatorOrder[validatorIndex]!=currentValidator) {
        validatorIndex++;
    }
    
    vector<Transaction> txs;
    if (validatorIndex < validatorTxsLists.size()) {
        txs = validatorTxsLists[validatorIndex];
    }

    if (attack ==2 && (validatorIndex == 1 || validatorIndex==2)) {
        vector<Transaction> filtered;
        cout << "transactions selected by miner " << validatorIndex << ":\n";
        for (const auto& tx : txs) {
            cout << "  " << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
        }
        for (const auto& tx : validatorTxsLists[validatorIndex]) {
            if (tx.sender != "1" && tx.receiver != "1") {
                filtered.push_back(tx);
            } else if(validatorIndex==1){
                cout << "[Miner 1] Skipping transaction involving user 1: " 
                     << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
            } else{
                cout << "[Miner 2] Skipping transaction involving user 1: " 
                     << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
            }
        }
        txs = filtered;
    } 
    
    cout << validator.name << " is creating a block...\n";
    Block newBlock = createBlock(currentValidator, blockchain.getTip(), txs);
    totalHashAttempts++;
    
    cout << "All miners are verifying the block...\n";
    vector<thread> verificationThreads;
    vector<bool> verificationResults(validatorOrder.size());
    for (size_t i = 0; i < validatorOrder.size();i++) {
        verificationThreads.emplace_back([&, i]() {
            verificationResults[i] = validateBlockAuthority(newBlock,validatorOrder[i], attack, i);
        });
    }

    for (auto& t : verificationThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    verifiedMiners = count(verificationResults.begin(), verificationResults.end(), true);
    
    if (verifiedMiners > totalMiners/2) {
        cout << "Block successfully validated by authorities!\n";
        blockchain.addBlock(newBlock);
        return newBlock;
    } else {
        cout << "[Validator " << validator.name << "] proposed an invalid block ";
        cout << "Block verification failed by some miners. It will not be added.\n";
        return Block{};
    }
}