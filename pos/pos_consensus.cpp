#include "pos_consensus.h"
#include "block.h"
#include "blockchain.h"
#include "transaction.h"
#include <random>
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <vector>
#include <atomic>

using namespace std;

int totalMiners; // Total number of miners
atomic<int> verifiedMiners(0);
atomic<long long> totalHashAttempts(0); //global count

bool isValidHash(const string& hash, const string& difficulty) {
    return hash.substr(0, difficulty.size()) == difficulty;
}

int selectValidator(const map<int, int>& stakes) {
    int totalStake = 0;
    for(auto& [minerId, stake] : stakes)
        totalStake += stake;

    if (totalStake == 0) {
        cout << "Error: Total stake is zero. Cannot select validator.\n";
        return -1; //halt the round
    }

    int randomValue = rand() % totalStake;
    int runningSum = 0;

    for(auto& [minerId, stake] : stakes) {
        runningSum += stake;
        if (randomValue < runningSum) return minerId;
    }

    return stakes.begin()->first; // fallback
}

void verifyBlock(const Blockchain& blockchain, const Block& block, int minerId,int attack) {
    cout << "[Miner " << minerId << "] verifying the block...\n";

    int attackThreshold = totalMiners / 2;

    if (attack ==1 && minerId >= attackThreshold) {
        cout << "[Miner " << minerId << "] Block verification ARTIFICIALLY failed! (51\% attack simulation)\n";
        return; // These miners don't increment verifiedMiners counter
    }

    if (isValidHash(block.hash, block.difficulty)) { // && minerId!=3 && minerId!=4 && minerId!=2
        cout << "[Miner " << minerId << "] Block verified successfully!\n";
        this_thread::sleep_for(chrono::milliseconds(1000));
        verifiedMiners++;
    } else {
        cout << "[Miner " << minerId << "] Block verification failed!\n";
    }
}

Block startStakingRound(Blockchain& blockchain, vector<vector<Transaction>>& minerTransactionList,map<int, int>& stakes, int attack) {
    totalMiners = stakes.size();
    verifiedMiners = 0;
    totalHashAttempts = 0;
    // long long localHashCount = 0;

    if (attack == 1) {
        int attackThreshold = totalMiners / 2;
        int attackingMiners = totalMiners - attackThreshold;
        cout << "ATTACK MODE ENABLED: " << attackingMiners << " miners will reject valid blocks\n";
        cout << "Attacking miners: " << attackingMiners << " (" 
                  << (attackingMiners * 100.0 / totalMiners) << "\% of network)\n";
    }

    int validator = selectValidator(stakes);
    vector<Transaction>& selectedTxs = minerTransactionList[validator];
    cout << "PoS: Miner " << validator << " selected to propose block.\n";
    
    cout << "transactions selected by miner " << validator << ":\n";
    for(const auto& tx : selectedTxs) {
        cout << "  " << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
    }

    if (attack ==2 && (validator == 1 || validator==2)) {
        vector<Transaction> filtered;
        for(const auto& tx : minerTransactionList[validator]) {
            if (tx.sender != "1" && tx.receiver != "1") {
                filtered.push_back(tx);
            } else if(validator==1){
                cout << "[Miner 1] Skipping transaction involving user 1: " 
                     << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
            } else{
                cout << "[Miner 2] Skipping transaction involving user 1: " 
                     << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
            }
        }
        selectedTxs = filtered;
    } 

    Block newBlock(blockchain.getTip(), selectedTxs);
    string difficulty = "0000"; 
    newBlock.difficulty = difficulty;
    totalHashAttempts ++;

    while(!isValidHash(newBlock.hash, difficulty)) {
        newBlock.nonce++;
        newBlock.hash = newBlock.calculateHash();
        totalHashAttempts ++;
    }

    
    cout << "[Validator " << validator << "] proposed block with hash: " << newBlock.hash << "\n";

    //After the block is found, have all miners verify it
    cout << "All miners are verifying the block...\n";
    vector<thread> verificationThreads;
    for(int i = 0; i < totalMiners; i++) {
        verificationThreads.emplace_back(verifyBlock, cref(blockchain), cref(newBlock), i,attack);
    }

    // Wait for all miners to finish their verification
    for(auto& t : verificationThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    double energyPerHash = 0.0000015; // joules per hash
    double totalEnergy = totalHashAttempts * energyPerHash;

    cout << "Total hash attempts: " << totalHashAttempts << "\n";
    cout << "Estimated energy consumed: " << totalEnergy << " joules\n";
    
    //Final check if miners verified the block
    if (verifiedMiners > totalMiners / 2) {
        blockchain.addBlock(newBlock);
        int reward = 5; 
        stakes[validator] += reward;
        cout << "[Validator " << validator << "] successfully mined the block and received " << reward << " reward.\n";
        cout << "Block successfully added to the blockchain!\n";
        return newBlock;
    } else {
        int penalty = 10;
        stakes[validator] = max(0, stakes[validator] - penalty);
        cout << "[Validator " << validator << "] proposed an invalid block and lost " << penalty << " due to penalty.\n";
        cout << "Block verification failed by some miners. It will not be added.\n";
        return Block{};
    }
}

