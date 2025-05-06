#include "poet_consensus.h"
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
#include <chrono>
#include <algorithm> // For find_if

using namespace std;

mutex mtx;
condition_variable cv;
atomic<bool> blockFound(false);
Block winningBlock;
atomic<int> verifiedMiners(0);
atomic<int> totalMiners(0);
atomic<int> minimumVerifications(0);
atomic<long long> totalHashAttempts(0); //global count

//In a real PoET implementation, this would use a trusted execution environment
bool verifyWaitCertificate(const Block& block) {
    // real implementation, we would verify with TEE :
    //The wait time was legitimately random
    //The miner actually waited the reported time
    //The certificate was issued by a valid TEE
    
    // For simulation purposes, we assume the wait certificate is always valid
    return true;
}

void minerWaitProcess(Blockchain& blockchain, vector<Transaction> txs, int minerId) {
    string parentHash = blockchain.getTip();
    
    // Generate a random wait time (for simulation purposes)
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> waitDist(1000, 10000); // Wait between 1-10 seconds
    int waitTime = waitDist(gen);
    
    cout << "[Miner " << minerId << "] received wait time of " << waitTime/1000.0 << " seconds...\n";
    
    // Create candidate block with PoET-specific constructor
    Block candidate(parentHash, txs, waitTime);
    
    // Wait for the random time
    auto start = chrono::steady_clock::now();
    long long localHashCount = 0;
    totalHashAttempts++;
    
    while (!blockFound.load()) {
        auto now = chrono::steady_clock::now();
        auto elapsedMs = chrono::duration_cast<chrono::milliseconds>(now - start).count();
        
        if (elapsedMs >= waitTime) {
            unique_lock<mutex> lock(mtx);
            if (!blockFound.load()) {
                blockFound = true;
                winningBlock = candidate;
                cout << "[Miner " << minerId << "] completed wait time first! Broadcasting block...\n";
                cv.notify_all();
            }
            return;
        }
        // Sleep for a small amount of time to reduce CPU usage
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

void verifyBlock(const Blockchain& blockchain, const Block& block, int minerId,int attack) {
    cout << "[Miner " << minerId << "] verifying the block...\n";

    int attackThreshold = totalMiners / 2;

    if (attack ==1 && minerId >= attackThreshold) {
        cout << "[Miner " << minerId << "] Block verification ARTIFICIALLY failed! (51\% attack simulation)\n";
        return; // These miners don't increment verifiedMiners counter
    }

    // Verify that the wait certificate is valid
    if (verifyWaitCertificate(block) && block.hash == block.calculateHash()) {
        cout << "[Miner " << minerId << "] Block verified successfully! Wait time was "
                  << block.waitTime/1000.0 << " seconds.\n";
        this_thread::sleep_for(chrono::milliseconds(500));
        verifiedMiners++;
    } else {
        cout << "[Miner " << minerId << "] Block verification failed! Invalid wait certificate.\n";
    }
}

Block startPoETRound(Blockchain& blockchain, vector<vector<Transaction>> minerTxsList,int attack) {
    totalHashAttempts = 0;
    totalMiners = minerTxsList.size();
    blockFound = false;
    verifiedMiners = 0;

    if (attack == 1) {
        int attackThreshold = totalMiners / 2;
        int attackingMiners = totalMiners - attackThreshold;
        cout << "ATTACK MODE ENABLED: " << attackingMiners << " miners will reject valid blocks\n";
        cout << "Attacking miners: " << attackingMiners << " (" 
                  << (attackingMiners * 100.0 / totalMiners) << "\% of network)\n";
    }
    
    vector<thread> miners;

    for(size_t i = 0; i < minerTxsList.size(); i++) {
        if (attack ==2 && (i == 1 || i==2)) {
            cout << "transactions selected by miner " << i << ":\n";
            for(const auto& tx : minerTxsList[i]) {
                cout << "  " << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
            }
            vector<Transaction> filtered;
            for(const auto& tx : minerTxsList[i]) {
                if (tx.sender != "1" && tx.receiver != "1") {
                    filtered.push_back(tx);
                } else if(i==1){
                    cout << "[Miner 1] Skipping transaction involving user 1: " 
                         << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
                } else{
                    cout << "[Miner 2] Skipping transaction involving user 1: " 
                         << tx.sender << " -> " << tx.receiver << " [" << tx.amount << "]\n";
                }
            }
            miners.emplace_back(minerWaitProcess, ref(blockchain), minerTxsList[i], i);
        } else {
            miners.emplace_back(minerWaitProcess, ref(blockchain), minerTxsList[i], i);
        }
    }

    {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [] { return blockFound.load(); });
    }

    // Join all miner threads
    for (auto& miner : miners) {
        if (miner.joinable()) {
            miner.join();
        }
    }

    // After the block is found, have all miners verify it
    cout << "All miners are verifying the block...\n";
    vector<thread> verificationThreads;
    for(int i = 0; i < totalMiners; i++) {
        verificationThreads.emplace_back(verifyBlock, cref(blockchain), cref(winningBlock), i,attack);
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

    if (verifiedMiners > totalMiners*1/2) {
        blockchain.addBlock(winningBlock);
        cout << "Block successfully added to the blockchain! (" << verifiedMiners << "/" << totalMiners << " verifications)\n";
    } else {
        cout << "Block verification failed: only " << verifiedMiners << "/" << totalMiners << " miners verified. It will not be added.\n";
    }

    return winningBlock;
}