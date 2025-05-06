#ifndef POS_CONSENSUS_H
#define POS_CONSENSUS_H

#include "blockchain.h"
#include "transaction.h"
#include <vector>
#include <map>
#include <atomic>

using namespace std;

    Block startStakingRound(Blockchain& blockchain,vector<vector<Transaction>>& minerTxsList,map<int, int>& stakes, int attack = 0);

    void verifyBlock(const Blockchain& blockchain, const Block& block, int minerId,int attack =0);
    bool isValidHash(const string& hash, const string& difficulty);
    int selectValidator(const map<int, int>& stakes);

#endif
