#ifndef POET_CONSENSUS_H
#define POET_CONSENSUS_H

#include "blockchain.h"
#include "transaction.h"
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

using namespace std;

bool verifyWaitCertificate(const Block& block);
void minerWaitProcess(Blockchain& blockchain, vector<Transaction> txs, int minerId);
void verifyBlock(const Blockchain& blockchain, const Block& block, int minerId,int attack=0);
Block startPoETRound(Blockchain& blockchain, vector<vector<Transaction>> minerTxsList, int attack=0);

#endif