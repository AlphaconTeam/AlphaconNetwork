// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2014 The BlackCoin developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACON_CONSENSUS_PARAMS_H
#define ALPHACON_CONSENSUS_PARAMS_H

#include "uint256.h"
#include "amount.h"
#include <map>
#include <string>

namespace Consensus {

enum DeploymentPos
{
    DEPLOYMENT_TESTDUMMY,
    DEPLOYMENT_TOKENS, // Deployment of RIP2
    // DEPLOYMENT_CSV, // Deployment of BIP68, BIP112, and BIP113.
//    DEPLOYMENT_SEGWIT, // Deployment of BIP141, BIP143, and BIP147.
    // NOTE: Also add new deployments to VersionBitsDeploymentInfo in versionbits.cpp
    MAX_VERSION_BITS_DEPLOYMENTS
};

/**
 * Struct for each individual consensus rule change using BIP9.
 */
struct BIP9Deployment {
    /** Bit position to select the particular bit in nVersion. */
    int bit;
    /** Start MedianTime for version bits miner confirmation. Can be a date in the past */
    int64_t nStartTime;
    /** Timeout/expiry MedianTime for the deployment attempt. */
    int64_t nTimeout;
};

/**
 * Parameters that influence chain consensus.
 */
struct Params {
    uint256 hashGenesisBlock;
    /** Block height and hash at which BIP34 becomes active */
    bool nBIP34Enabled;
    bool nBIP65Enabled;
    bool nBIP66Enabled;
    // uint256 BIP34Hash;
    /** Block height at which BIP65 becomes active */
    // int BIP65Height;
    /** Block height at which BIP66 becomes active */
    // int BIP66Height;
    /**
     * Minimum blocks including miner confirmation of the total of 2016 blocks in a retargeting period,
     * (nTargetTimespan / nTargetSpacing) which is also used for BIP9 deployments.
     * Examples: 1916 for 95%, 1512 for testchains.
     */
    uint32_t nRuleChangeActivationThreshold;
    uint32_t nMinerConfirmationWindow;
    BIP9Deployment vDeployments[MAX_VERSION_BITS_DEPLOYMENTS];
    /** Proof of work parameters */
    uint256 powLimit;
    uint256 posLimit;
    int64_t nTargetSpacing;
    int64_t nTargetTimespan;
    int64_t DifficultyAdjustmentInterval() const { return nTargetTimespan / nTargetSpacing; }
    uint256 nMinimumChainWork;
    uint256 defaultAssumeValid;
    bool nSegwitEnabled;
    bool nCSVEnabled;
    
    // ALP
    int nLastPOWBlock;
    CAmount nBlockRewardALP;
    int nRewardHeighALP;
    CAmount nBlockReward;
    int nBlockRewardHalvings;
    int nBlockRewardHalvingsWindow;
    int nTokensDeploymentHeight;

    int nStakeTimestampMask;
    int nCoinbaseMaturity;
    int nStakeMaturity;
};
} // namespace Consensus

#endif // ALPHACON_CONSENSUS_PARAMS_H
