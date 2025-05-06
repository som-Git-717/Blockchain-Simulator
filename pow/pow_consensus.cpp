#include "pow_consensus.h"
#include "blockchain.h"
#include "transaction.h"
#include <iostream>
#include <ctime>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <random>

using namespace std;

mutex mtx;
condition_variable cv;
atomic<bool> blockFound(false);
Block winningBlock;
atomic<int> verifiedMiners(0); // Count of miners who verified the block
atomic<int> totalMiners(0); // Total number of miners
atomic<long long> totalHashAttempts(0); //global count
atomic<int> sybilMinerId(-1); // stores the ID of the miner who found the block in a Sybil attack

bool isValidHash(const string& hash, const string& difficulty) {
    return hash.substr(0, difficulty.size()) == difficulty;
}

void mineBlock(Blockchain& blockchain, vector<Transaction> transactions, string difficulty, int minerId, bool isSybil) {
    string parentHash = blockchain.getTip();
    int nonce = 0;
    time_t timestamp = time(nullptr);
    long long localHashCount = 0;

    while (!blockFound.load()) {
        Block candidate;
        candidate.parentHash = parentHash;
        candidate.transactions = transactions;
        candidate.timestamp = timestamp;
        candidate.nonce = nonce++;
        candidate.difficulty = difficulty;
        candidate.merkleRoot = Block::calculateMerkleRoot(candidate.transactions);
        candidate.hash = candidate.calculateHash();

        localHashCount++;

        if (isValidHash(candidate.hash, difficulty)) {
            unique_lock<mutex> lock(mtx);
            if (!blockFound.load()) {
                blockFound = true;
                winningBlock = candidate;
                if (isSybil) {
                    sybilMinerId.store(minerId);
                }
                cout << "[Miner " << minerId << "] found a block! Broadcasting...\n";
                cv.notify_all();
            }
            totalHashAttempts += localHashCount;
            return;
        }
    }
    totalHashAttempts += localHashCount;
}

void verifyBlock(const Blockchain& blockchain, const Block& block, int minerId, int attack) {
    cout << "[Miner " << minerId << "] verifying the block...\n";
    
    //In 51% attack mode, more than half of the miners will reject the block
    if (attack == 1) {
        int attackThreshold = totalMiners/2;
        if (minerId >= attackThreshold) {
            cout << "[Miner " << minerId << "] Block verification ARTIFICIALLY failed! (51\% attack simulation)\n";
            return; // These miners don't increment verifiedMiners counter
        }
    }
    
    // In Sybil attack mode, check if this is a Sybil miner and it found the block
    if (attack == 3) {
        // Assuming miners with high IDs are Sybil miners
        int regularMiners = totalMiners/3; // Assume 1/3 are regular miners, 2/3 are Sybil
        bool isSybilMiner = (minerId >= regularMiners);
        int sybilMinerWhoFoundBlock = sybilMinerId.load();
        
        // Sybil miners always approve blocks from other Sybil miners
        if (isSybilMiner && sybilMinerWhoFoundBlock >= regularMiners && sybilMinerWhoFoundBlock >= 0) {
            cout << "[Miner " << minerId << "] (Sybil) Automatically approving block from Sybil miner " 
                      << sybilMinerWhoFoundBlock << "!\n";
            this_thread::sleep_for(chrono::milliseconds(500));
            verifiedMiners++;
            return;
        }
    }
    
    if (isValidHash(block.hash, block.difficulty)) {
        cout << "[Miner " << minerId << "] Block verified successfully!\n";
        this_thread::sleep_for(chrono::milliseconds(1000));
        verifiedMiners++;
    } else {
        cout << "[Miner " << minerId << "] Block verification failed!\n";
    }
}

Block startMiningRound(Blockchain& blockchain, vector<vector<Transaction>> minerTransactionList, string difficulty, int attack) {
    blockFound = false;
    verifiedMiners = 0;
    totalMiners = minerTransactionList.size();
    totalHashAttempts = 0;
    sybilMinerId.store(-1); // Reset the Sybil miner ID
    cout << "Total miners: " << totalMiners << "\n";

    if (attack == 1) {
        int attackThreshold = totalMiners / 2;
        int attackingMiners = totalMiners - attackThreshold;
        cout << "ATTACK MODE ENABLED: " << attackingMiners << " miners will reject valid blocks\n";
        cout << "Attacking miners: " << attackingMiners << " (" 
                  << (attackingMiners * 100.0 / totalMiners) << "\% of network)\n";
    } else if (attack == 2) {
        cout << "ATTACK MODE ENABLED: DoS attack - some miners will exclude transactions\n";
    } else if (attack == 3) {
        int regularMiners = totalMiners / 3; // Assume 1/3 are regular miners, 2/3 are Sybil
        int sybilMiners = totalMiners - regularMiners;
        cout << "ATTACK MODE ENABLED: Sybil attack - " << sybilMiners << " miners (" 
                  << (sybilMiners * 100.0 / totalMiners) << "\% of network) are controlled by attacker\n";
    }

    vector<thread> miners;

    for (size_t i = 0; i < minerTransactionList.size(); i++) {
        if (attack == 2 && (i == 1 || i == 2)) {
            vector<Transaction> filtered;
            for (const auto& tx : minerTransactionList[i]) {
                if (tx.sender != "1" && tx.receiver != "1") {
                    filtered.push_back(tx);
                } else if(i == 1){
                    cout << "[Miner 1] Skipping transaction involving user 1: " 
                         << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
                } else{
                    cout << "[Miner 2] Skipping transaction involving user 1: " 
                         << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
                }
            }
            miners.emplace_back(mineBlock, ref(blockchain), filtered, difficulty, i, false);
        } else if (attack == 3) {
            int regularMiners = totalMiners / 3; // Assume 1/3 are regular miners, 2/3 are Sybil
            bool isSybilMiner = (i >= static_cast<size_t>(regularMiners));
            
            // If Sybil miner, they might prioritize certain transactions or exclude others
            if (isSybilMiner) {
                miners.emplace_back(mineBlock, ref(blockchain), minerTransactionList[i], difficulty, i, true);
            } else {
                miners.emplace_back(mineBlock, ref(blockchain), minerTransactionList[i], difficulty, i, false);
            }
        } else {
            miners.emplace_back(mineBlock, ref(blockchain), minerTransactionList[i], difficulty, i, false);
        }    
    }

    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return blockFound.load(); });
    }

    //Join all miner threads
    for (auto& miner : miners) {
        if (miner.joinable()) {
            miner.join();
        }
    }

    //After the block is found, have all miners verify it
    cout << "All miners are verifying the block...\n";
    vector<thread> verificationThreads;
    for (int i = 0; i < totalMiners; i++) {
        verificationThreads.emplace_back(verifyBlock, cref(blockchain), cref(winningBlock), i, attack);
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

    //Final check if miners verified the block
    if (verifiedMiners > totalMiners / 2) {
        blockchain.addBlock(winningBlock);
        cout << "Block successfully added to the blockchain!\n";
        
        if (attack == 3 && sybilMinerId.load() >= 0) {
            int regularMiners = totalMiners / 3;
            if (sybilMinerId.load() >= regularMiners) {
                cout << "SYBIL ATTACK SUCCESSFUL: Block from Sybil miner " << sybilMinerId.load() 
                          << " was accepted by the network!\n";
                
                // Count how many transactions in the winning block are from the attacker
                int attackerTxCount = 0;
                for(const auto& tx : winningBlock.transactions) {
                    // We consider transactions from a specific user as attacker's
                    if (tx.sender == "5" || tx.receiver == "5") {
                        attackerTxCount++;
                    }
                }
                
                if (attackerTxCount > 0) {
                    cout << "The attacker successfully included " << attackerTxCount 
                              << " of their transactions in the blockchain!\n";
                }
            }
        }
        
        return winningBlock;
    } else {
        cout << "Block verification failed by majority miners. It will not be added.\n";
        return Block{};
    }
}