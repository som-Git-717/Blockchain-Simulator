#ifndef POW_CONSENSUS_H
#define POW_CONSENSUS_H

#include "blockchain.h"
#include "transaction.h"
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace std;

extern void minerFunction(Blockchain& blockchain, int minerId, const vector<Transaction>& txs);
extern void addBlockToBlockchain(Blockchain& blockchain, const Block& block);

//signatures of functions used in pow_consensus
bool isValidHash(const string& hash, const string& difficulty);
void mineBlock(Blockchain& blockchain, vector<Transaction> txs, string difficulty, int minerId, bool isSybil = false);
void verifyBlock(const Blockchain& blockchain, const Block& block, int minerId, int attack = 0);
Block startMiningRound(Blockchain& blockchain, vector<vector<Transaction>> minerTransactionList, string difficulty, int attack = 0);

#endif