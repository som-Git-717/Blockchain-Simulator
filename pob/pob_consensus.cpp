#include "pob_consensus.h"
#include <random>
#include <iostream>
#include <cmath>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <map>
#include <atomic>
#include <ctime>
#include <algorithm>


using namespace std;

int totalMiners; // Total number of miners
atomic<int> verifiedMiners(0);
atomic<long long> totalHashAttempts(0); //global count

bool isValidHash(const string& hash, const string& difficulty) {
    return hash.substr(0, difficulty.size()) == difficulty;
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

double calculateEffectiveBurnPower(const vector<BurnRecord>& records) {
    double totalPower = 0.0;
    time_t currentTime = time(nullptr);
    
    // Calculate the burn power with a time decay factor
    //More recent burns have more weight
    for(const auto& record : records) {
        //linear decay over 100milliseconds
        double ageFactor = 1.0 - min(1.0, double(currentTime - record.timestamp)/ 0.1);
        totalPower += record.burnedCoins * ageFactor;
    }
    
    return totalPower;
}

int selectValidator(const map<int, vector<BurnRecord>>& burnRecords) {
    // Calculate effective burn power for each miner
    map<int, double> effectivePower;
    double totalPower = 0.0;
    
    for(const auto& [minerId, records] : burnRecords) {
        effectivePower[minerId] = calculateEffectiveBurnPower(records);
        totalPower += effectivePower[minerId];
    }
    
    // If no one has burned coins, select randomly
    if (totalPower <= 0) {
        int randomMiner = rand() % burnRecords.size();
        auto it = burnRecords.begin();
        advance(it, randomMiner);
        return it->first;
    }
    
    // Weighted random selection based on burned coins
    double randomValue = (double)rand() / RAND_MAX * totalPower;
    double runningSum = 0.0;
    
    for(const auto& [minerId, power] : effectivePower) {
        runningSum += power;
        if (randomValue < runningSum) return minerId;
    }
    
    return burnRecords.begin()->first; // fallback
}

Block startBurningRound(Blockchain& blockchain, vector<vector<Transaction>>& minerTxsList, map<int, vector<BurnRecord>>& burnRecords,int attack) {
    totalHashAttempts = 0;
    totalMiners = burnRecords.size();
    verifiedMiners = 0;

    //This simulates miners burning coins to participate
    for (size_t i = 0; i < minerTxsList.size(); i++) {
        // Each miner burns a random amount between 1 and 10 coins
        double burnAmount = 1.0 + (rand() % 10);
        cout << "Miner " << i << " burns " << burnAmount << " coins\n";
        
        // Record the burn
        burnRecords[i].push_back(BurnRecord(burnAmount, time(nullptr)));
        
        //Create a burn transaction->sending to a burn address
        Transaction burnTx("Miner" + to_string(i), "BURN_ADDRESS", burnAmount);
        //In a real implementation, you would add this to the transaction pool
    }
    
        // Display total burns for each miner after this round
        cout << "\nCurrent burn totals:\n";
        for (int i = 0; i < totalMiners; i++) {
            double totalBurned = 0;
            for (const auto& record : burnRecords[i]) {
                totalBurned += record.burnedCoins;
            }
            cout << "Miner " << i << " has burned a total of " << totalBurned << " coins\n";
            cout << "Effective burn power: " << calculateEffectiveBurnPower(burnRecords[i]) << "\n";
        }

    if (attack == 1) {
        int attackThreshold = totalMiners / 2;
        int attackingMiners = totalMiners - attackThreshold;
        cout << "ATTACK MODE ENABLED: " << attackingMiners << " miners will reject valid blocks\n";
        cout << "Attacking miners: " << attackingMiners << " (" 
                    << (attackingMiners * 100.0 / totalMiners) << "\% of network)\n";
    }

    // Select a validator based on burn records
    int validator = selectValidator(burnRecords);
    vector<Transaction>& selectedTxs = minerTxsList[validator];
    
    cout << "PoB: Miner " << validator << " selected to propose block based on burned coins.\n";

    cout << "transactions selected by miner " << validator << ":\n";
    for (const auto& tx : selectedTxs) {
        cout << "  " << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
    }

    if (attack ==2 && (validator == 1 || validator==2)) {
        vector<Transaction> filtered;
        for (const auto& tx : minerTxsList[validator]) {
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
    
    // Create new block with the validator's transactions
    Block newBlock(blockchain.getTipHash(), selectedTxs);
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
    for (auto& t : verificationThreads) {
        if (t.joinable()) {
            t.join();
        }
    }

    double energyPerHash = 0.0000015; // joules per hash
    double totalEnergy = totalHashAttempts * energyPerHash;

    cout << "Total hash attempts: " << totalHashAttempts << "\n";
    cout << "Estimated energy consumed: " << totalEnergy << " joules\n";
    
    if (verifiedMiners > totalMiners/2) {
        blockchain.addBlock(newBlock);
        cout << "verifiedMiners: " << verifiedMiners << "\n";
        cout << "totalMiners/2: " << totalMiners/2 << "\n";
        cout << "PoB: Block accepted and added by validator Miner " << validator << "\n";
        cout << "Effective burn power: " << calculateEffectiveBurnPower(burnRecords[validator]) << "\n";
        return newBlock;
    } else {
        cout << "[Validator " << validator << "] proposed an invalid block ";
        cout << "Block verification failed by some miners. It will not be added.\n";
        return Block{};
    }
}

