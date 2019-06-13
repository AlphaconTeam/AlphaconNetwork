// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2014 The BlackCoin developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"
#include "validation.h"
#include "chainparams.h"
#include "tinyformat.h"

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    unsigned int nTargetLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL) {
        return nTargetLimit;
    }

    const CBlockIndex* pindexPrev = pindexLast;
    if (pindexPrev->pprev == NULL) {
        return nTargetLimit; // first block
    }

    const CBlockIndex* pindexPrevPrev = pindexPrev->pprev;
    if (pindexPrevPrev->pprev == NULL) {
        return nTargetLimit; // second block
    }

    return CalculateNextTargetRequired(pindexPrev, pindexPrevPrev->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nTargetTimespan/4)
        nActualTimespan = params.nTargetTimespan/4;
    if (nActualTimespan > params.nTargetTimespan*4)
        nActualTimespan = params.nTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

/* Proof-of-Stake */
static arith_uint256 GetTargetLimit(int64_t nTime, bool fProofOfStake, const Consensus::Params& params)
{
    uint256 nLimit;

    if (fProofOfStake) {
        nLimit = params.posLimit;
    } else {
        nLimit = params.powLimit;
    }

    return UintToArith256(nLimit);
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, bool fProofOfStake, const Consensus::Params& params)
{
    unsigned int nTargetLimit = UintToArith256(fProofOfStake ? params.posLimit : params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL) {
        return nTargetLimit;
    }

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == NULL) {
        return nTargetLimit; // first block
    }

    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == NULL) {
        return nTargetLimit; // second block
    }

    return CalculateNextTargetRequired(pindexPrev, pindexPrevPrev->GetBlockTime(), params);
}

unsigned int CalculateNextTargetRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    int64_t nActualSpacing = pindexLast->GetBlockTime() - nFirstBlockTime;
    int64_t nTargetSpacing = params.nTargetSpacing;

    // Limit adjustment step
    if (nActualSpacing < 0) {
        nActualSpacing = nTargetSpacing;
    }

    if (nActualSpacing > nTargetSpacing * 10) {
        nActualSpacing = nTargetSpacing * 10;
    }

    // retarget with exponential moving toward target spacing
    const arith_uint256 bnTargetLimit = GetTargetLimit(pindexLast->GetBlockTime(), pindexLast->IsProofOfStake(), params);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    int64_t nInterval = params.nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}
/* Proof-of-Stake */

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
