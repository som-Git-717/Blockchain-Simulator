#include "blockchain.h"
#include "transaction.h"
#include "poa_consensus.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <iostream>
#include <chrono>
#include <algorithm>

using namespace std;

extern atomic<long long> totalHashAttempts;

int main(int argc, char* argv[]) {

    int attack = 0;
    if (argc > 1 && string(argv[1]) == "--51attack") {
        attack = 1;
    } else if(argc > 1 && string(argv[1]) == "--DoSattack"){
        attack = 2;
    }

    Blockchain blockchain;
    int minerCount = 5; // how many miners per round
    int totalRounds = 1000; // how many blocks to mine
    int txsPerMiner = 3;    // max transactions per block
    int count=100;        // how many transactions to generate

    vector<Transaction> transactionPool;
    srand(time(nullptr));
    int minAmount = 10, maxAmount = 100;
    
    for(int i = 1; i <= count; i++) {
        int amount = minAmount + rand() % (maxAmount - minAmount + 1);
        transactionPool.emplace_back(to_string(i), to_string(i + 1), amount);
    }

    string consensusAlgo = "PoA";
    cout << "Using Proof of Authority consensus algorithm.\n";

    int totalTransactions = transactionPool.size();
    long long globalHashAttempts = 0; // accumulate total hashes over all rounds
    
    PoAConsensus poaConsensus(minerCount);

    random_device rd;
    mt19937 gen(rd());

    if (attack == 1) {
        cout << "\n SIMULATING 51\% ATTACK \n";
        int attackingMiners = minerCount - (minerCount / 2);
        cout << "Attacking miners: " << attackingMiners << " (" << (attackingMiners * 100.0 / minerCount) << "% of network)\n\n";
    } 
    if (attack == 2) {
        cout << "\n SIMULATING DoS ATTACK \n";
        cout << "Miner 2 and Miner 3 will skip transactions involving user 1.\n\n";
    }

    auto startTime = chrono::high_resolution_clock::now();

    for (int round = 0; round < totalRounds && !transactionPool.empty(); round++) {
        cout << "\n=== Round " << round + 1 << " ===\n";
    
        vector<vector<Transaction>> validatorTxs;
    
        for(int i = 0; i < minerCount;i++) {
            vector<Transaction> chosen;
            if (!transactionPool.empty()) {
                int pickCount = min(txsPerMiner, (int)transactionPool.size());
        
                // Create a copy of the indices and shuffle to sample without replacement
                vector<int> indices(transactionPool.size());
                iota(indices.begin(), indices.end(), 0);  // Fill with 0, 1, ..., n-1
                shuffle(indices.begin(), indices.end(), gen);
        
                for(int j = 0; j < pickCount;j++) {
                    chosen.push_back(transactionPool[indices[j]]);
                }
            }
            validatorTxs.push_back(chosen);
        }
    
            Block addedBlock = poaConsensus.startBlockCreationRound(blockchain, validatorTxs,attack);
            // blockchain.addBlock(addedBlock);
            
            // Remove only transactions from the winning block
            if (!addedBlock.hash.empty()) {
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
            } else {
                cout << "No block was added this round due to verification failure (attack effect)\n";
            }
        globalHashAttempts += totalHashAttempts.load();
        cout << "transactions left: " << transactionPool.size() << "\n";
        // for(const auto& tx : transactionPool) {
        //     cout << "  " << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
        // }
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

    blockchain.displayBlockchainHashes();


    return 0;
}