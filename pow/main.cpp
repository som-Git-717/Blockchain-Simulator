#include "blockchain.h"
#include "transaction.h"
#include "pow_consensus.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <string>
#include <cstdlib>
#include <ctime>

using namespace std;

extern atomic<long long> totalHashAttempts;

int main(int argc, char* argv[]){

    int attack = 0;
    if (argc > 1 && string(argv[1]) == "--51attack") {
        attack = 1;
    } else if(argc > 1 && string(argv[1]) == "--DoSattack"){
        attack = 2;
    } else if(argc > 1 && string(argv[1]) == "--sybilattack"){
        attack = 3;
    }
    Blockchain blockchain;
    int minerCount = 5; // how many miners per round
    int totalRounds = 1000; // how many blocks to mine
    int txsPerMiner = 3;    // max transactions per block
    int count=100;        // how many transactions to generate

    std::vector<Transaction> transactionPool;
    std::srand(std::time(nullptr)); // Seed random generator
    int minAmount = 10, maxAmount = 100;
    for (int i = 1; i <= count; ++i) {
        int amount = minAmount + std::rand() % (maxAmount - minAmount + 1);
        transactionPool.emplace_back(std::to_string(i), std::to_string(i + 1), amount);
    }
    
    // Add some transactions from the attacker (assuming user ID 5 is the attacker)
    if (attack == 3) {
        for (int i = 1; i <= 5; ++i) {
            int amount = minAmount + std::rand() % (maxAmount - minAmount + 1);
            transactionPool.emplace_back("5", std::to_string(i), amount);
        }
    }

    int totalTransactions = transactionPool.size();
    long long globalHashAttempts = 0; // accumulate total hashes over all rounds

    
    // For Sybil attack, increase the number of miners with most controlled by attacker
    if (attack == 3) {
        minerCount = 15;  // Increase total miners, with majority being Sybil miners
        cout << "\n SIMULATING SYBIL ATTACK \n";
        int regularMiners = minerCount / 3;  // Only 1/3 are legitimate miners
        int sybilMiners = minerCount - regularMiners;
        cout << "Total miners: " << minerCount << "\n";
        cout << "Regular miners: " << regularMiners << " (" << (regularMiners * 100.0 / minerCount) << "% of network)\n";
        cout << "Sybil miners controlled by attacker: " << sybilMiners << " (" << (sybilMiners * 100.0 / minerCount) << "% of network)\n\n";
    }
    
    string difficulty = "0000";

    random_device rd;
    mt19937 gen(rd());

    cout << "Using Proof of Work consensus algorithm.\n";

    if (attack == 1) {
        cout << "\n SIMULATING 51\% ATTACK \n";
        int attackingMiners = minerCount - (minerCount / 2);
        cout << "Attacking miners: " << attackingMiners << " (" << (attackingMiners * 100.0 / minerCount) << "% of network)\n\n";
    } 

    auto startTime = chrono::high_resolution_clock::now();// start time
    int blocksAdded = 0; // Keep track of blocks added to the chain

    for (int round = 0; round < totalRounds && !transactionPool.empty(); round++) {
        cout << "\n=== Round " << round + 1 << " ===\n";
    
        vector<vector<Transaction>> minerTxs;
    
        //prepare transactions for each miner
        for (int i = 0; i < minerCount; i++) {
            vector<Transaction> chosen;
            
            if (attack == 3) {
                int regularMiners = minerCount / 3;
                
                // If this is a Sybil miner, they might prioritize attacker's transactions
                if (i >= regularMiners) {
                    // For Sybil miners, prioritize transactions from the attacker (user 5)
                    for (size_t j = 0; j < transactionPool.size(); j++) {
                        if (transactionPool[j].sender == "5" || transactionPool[j].receiver == "5") {
                            chosen.push_back(transactionPool[j]);
                            if (chosen.size() >= static_cast<size_t>(txsPerMiner)) {
                                break;
                            }
                        }
                    }
                    
                    // If we still need more transactions, add random ones
                    if (chosen.size() < static_cast<size_t>(txsPerMiner) && !transactionPool.empty()) {
                        int remainingToSelect = min(txsPerMiner - static_cast<int>(chosen.size()), static_cast<int>(transactionPool.size()));
                        
                        vector<int> indices(transactionPool.size());
                        iota(indices.begin(), indices.end(), 0);
                        shuffle(indices.begin(), indices.end(), gen);
                        
                        for(int j = 0; j < remainingToSelect; j++) {
                            const Transaction& tx = transactionPool[indices[j]];
                            // Check if this transaction is not already selected
                            auto it = find(chosen.begin(), chosen.end(), tx);
                            if (it == chosen.end()) {
                                chosen.push_back(tx);
                            }
                        }
                    }
                }
                else {
                    // Regular miners select transactions randomly
                    if (!transactionPool.empty()) {
                        int pickCount = min(txsPerMiner, static_cast<int>(transactionPool.size()));
                        
                        vector<int> indices(transactionPool.size());
                        iota(indices.begin(), indices.end(), 0);
                        shuffle(indices.begin(), indices.end(), gen);
                        
                        for(int j = 0; j < pickCount; j++) {
                            chosen.push_back(transactionPool[indices[j]]);
                        }
                    }
                }
            }
            else {
                // Normal transaction selection for non-Sybil attack scenarios
                if (!transactionPool.empty()) {
                    int pickCount = min(txsPerMiner, static_cast<int>(transactionPool.size()));
                    
                    vector<int> indices(transactionPool.size());
                    iota(indices.begin(), indices.end(), 0);
                    shuffle(indices.begin(), indices.end(), gen);
                    
                    for(int j = 0; j < pickCount; j++) {
                        chosen.push_back(transactionPool[indices[j]]);
                    }
                }
            }
            
            minerTxs.push_back(chosen);
        }

        // Start mining and get the block that was added
        Block addedBlock = startMiningRound(blockchain, minerTxs, difficulty, attack);

        globalHashAttempts += totalHashAttempts.load();
    
        // Remove only transactions in the winning block
        if (!addedBlock.hash.empty()) {
            blocksAdded++; // Increment block count
            for(const auto& tx : addedBlock.transactions) {
                auto it = find_if(transactionPool.begin(), transactionPool.end(),
                                  [&](const Transaction& t) {
                                      return t.sender == tx.sender &&
                                             t.receiver == tx.receiver &&
                                             t.amount == tx.amount;
                                  });
                if (it != transactionPool.end()) {
                    transactionPool.erase(it);
                }
            }
            
            if (attack == 3) {
                // In Sybil attack, check how many of attacker's transactions were included
                int attackerTxCount = 0;
                for(const auto& tx : addedBlock.transactions) {
                    if (tx.sender == "5" || tx.receiver == "5") {
                        attackerTxCount++;
                    }
                }
                
                if (attackerTxCount > 0) {
                    cout << "Round " << round + 1 << " result: " << attackerTxCount 
                         << " of the attacker's transactions were included\n";
                }
            }
        } else {
            cout << "No block was added this round due to verification failure (attack effect)\n";
        }
    }

    auto endTime = chrono::high_resolution_clock::now(); //end time
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);
    double throughput = (totalTransactions * 1000.0) / duration.count(); // calculate throughput
    double averageLatency = (duration.count()/1000) / static_cast<double>(totalTransactions); // calculate average latency
    cout << "\n\nTotal transactions: " << totalTransactions << "\n";
    cout << "Total time taken: " << duration.count() << " ms\n";
    cout << "Throughput: " << throughput << " transactions/sec\n";
    cout << "Average Latency: " << averageLatency << " sec per transaction" << endl;

    double energyPerHash = 0.0000015; // joules per hash
    double totalEnergy = globalHashAttempts * energyPerHash;

    // cout << "Total hash attempts (all rounds): " << globalHashAttempts << "\n";
    cout << "Estimated total energy consumed: " << totalEnergy << " joules\n";

    
    if (attack == 3) {
        
        // Count how many blocks in the chain came from Sybil miners
        int regularMiners = minerCount / 3;
        int sybilBlocks = 0;
        int totalBlocks = blocksAdded; 
        
        double sybilProbability = (double)(minerCount - regularMiners) / minerCount;
        sybilBlocks = round(totalBlocks * sybilProbability);
        
        cout << "\nTotal blocks in the chain excluding gensis: " << totalBlocks << "\n";
        cout << "Estimated blocks mined by Sybil attackers: " << sybilBlocks 
        << " (" << (sybilBlocks * 100.0 / totalBlocks) << "% of blocks)\n";
        cout << "The attacker controls " << (minerCount - regularMiners) << " out of " << minerCount << " miners\n";
        blockchain.displayBlockchainHashes();
    }

    return 0;
}