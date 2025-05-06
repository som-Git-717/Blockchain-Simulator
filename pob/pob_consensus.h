#ifndef POB_CONSENSUS_H
#define POB_CONSENSUS_H

#include "blockchain.h"
#include "transaction.h"
#include <vector>
#include <map>

using namespace std;


    // Structure to track burned coins for each miner
    struct BurnRecord{
        double burnedCoins;
        time_t timestamp;
        
        BurnRecord(double coins, time_t time) : burnedCoins(coins), timestamp(time) {}
    };
    
    // Select a validator based on burned coins
    int selectValidator(const map<int, vector<BurnRecord>>& burnRecords);
    
    // Start a burning round
    Block startBurningRound(Blockchain& blockchain, vector<vector<Transaction>>& minerTxsList,map<int, vector<BurnRecord>>& burnRecords,int attack=0);
    
    // Calculate effective burn power (decays over time)
    double calculateEffectiveBurnPower(const vector<BurnRecord>& records);
    void verifyBlock(const Blockchain& blockchain, const Block& block, int minerId, int attack=0);


#endif