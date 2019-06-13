// Copyright (c) 2017-2017 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tokens/tokens.h>
#include <script/standard.h>
#include <util.h>
#include <validation.h>
#include "tx_verify.h"

#include "consensus.h"
#include "chainparams.h"
#include "primitives/transaction.h"
#include "script/interpreter.h"
#include "validation.h"
#include <cmath>

// TODO remove the following dependencies
#include "chain.h"
#include "coins.h"
#include "utilmoneystr.h"

bool IsFinalTx(const CTransaction &tx, int nBlockHeight, int64_t nBlockTime)
{
    if (tx.nLockTime == 0)
        return true;
    if ((int64_t)tx.nLockTime < ((int64_t)tx.nLockTime < LOCKTIME_THRESHOLD ? (int64_t)nBlockHeight : nBlockTime))
        return true;
    for (const auto& txin : tx.vin) {
        if (!(txin.nSequence == CTxIn::SEQUENCE_FINAL))
            return false;
    }
    return true;
}

std::pair<int, int64_t> CalculateSequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    assert(prevHeights->size() == tx.vin.size());

    // Will be set to the equivalent height- and time-based nLockTime
    // values that would be necessary to satisfy all relative lock-
    // time constraints given our view of block chain history.
    // The semantics of nLockTime are the last invalid height/time, so
    // use -1 to have the effect of any height or time being valid.
    int nMinHeight = -1;
    int64_t nMinTime = -1;

    // tx.nVersion is signed integer so requires cast to unsigned otherwise
    // we would be doing a signed comparison and half the range of nVersion
    // wouldn't support BIP 68.
    bool fEnforceBIP68 = false;

    // Do not enforce sequence numbers as a relative lock time
    // unless we have been instructed to
    if (!fEnforceBIP68) {
        return std::make_pair(nMinHeight, nMinTime);
    }

    for (size_t txinIndex = 0; txinIndex < tx.vin.size(); txinIndex++) {
        const CTxIn& txin = tx.vin[txinIndex];

        // Sequence numbers with the most significant bit set are not
        // treated as relative lock-times, nor are they given any
        // consensus-enforced meaning at this point.
        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_DISABLE_FLAG) {
            // The height of this input is not relevant for sequence locks
            (*prevHeights)[txinIndex] = 0;
            continue;
        }

        int nCoinHeight = (*prevHeights)[txinIndex];

        if (txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_TYPE_FLAG) {
            int64_t nCoinTime = block.GetAncestor(std::max(nCoinHeight-1, 0))->GetPastTimeLimit();
            // NOTE: Subtract 1 to maintain nLockTime semantics
            // BIP 68 relative lock times have the semantics of calculating
            // the first block or time at which the transaction would be
            // valid. When calculating the effective block time or height
            // for the entire transaction, we switch to using the
            // semantics of nLockTime which is the last invalid block
            // time or height.  Thus we subtract 1 from the calculated
            // time or height.

            // Time-based relative lock-times are measured from the
            // smallest allowed timestamp of the block containing the
            // txout being spent, which is the median time past of the
            // block prior.
            nMinTime = std::max(nMinTime, nCoinTime + (int64_t)((txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) << CTxIn::SEQUENCE_LOCKTIME_GRANULARITY) - 1);
        } else {
            nMinHeight = std::max(nMinHeight, nCoinHeight + (int)(txin.nSequence & CTxIn::SEQUENCE_LOCKTIME_MASK) - 1);
        }
    }

    return std::make_pair(nMinHeight, nMinTime);
}

bool EvaluateSequenceLocks(const CBlockIndex& block, std::pair<int, int64_t> lockPair)
{
    assert(block.pprev);
    int64_t nBlockTime = block.pprev->GetPastTimeLimit();
    if (lockPair.first >= block.nHeight || lockPair.second >= nBlockTime)
        return false;

    return true;
}

bool SequenceLocks(const CTransaction &tx, int flags, std::vector<int>* prevHeights, const CBlockIndex& block)
{
    return EvaluateSequenceLocks(block, CalculateSequenceLocks(tx, flags, prevHeights, block));
}

unsigned int GetLegacySigOpCount(const CTransaction& tx)
{
    unsigned int nSigOps = 0;
    for (const auto& txin : tx.vin)
    {
        nSigOps += txin.scriptSig.GetSigOpCount(false);
    }
    for (const auto& txout : tx.vout)
    {
        nSigOps += txout.scriptPubKey.GetSigOpCount(false);
    }
    return nSigOps;
}

unsigned int GetP2SHSigOpCount(const CTransaction& tx, const CCoinsViewCache& inputs)
{
    if (tx.IsCoinBase())
        return 0;

    unsigned int nSigOps = 0;
    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        if (prevout.scriptPubKey.IsPayToScriptHash())
            nSigOps += prevout.scriptPubKey.GetSigOpCount(tx.vin[i].scriptSig);
    }
    return nSigOps;
}

int64_t GetTransactionSigOpCost(const CTransaction& tx, const CCoinsViewCache& inputs, int flags)
{
    int64_t nSigOps = GetLegacySigOpCount(tx) * WITNESS_SCALE_FACTOR;

    if (tx.IsCoinBase())
        return nSigOps;

    if (flags & SCRIPT_VERIFY_P2SH) {
        nSigOps += GetP2SHSigOpCount(tx, inputs) * WITNESS_SCALE_FACTOR;
    }

    for (unsigned int i = 0; i < tx.vin.size(); i++)
    {
        const Coin& coin = inputs.AccessCoin(tx.vin[i].prevout);
        assert(!coin.IsSpent());
        const CTxOut &prevout = coin.out;
        nSigOps += CountWitnessSigOps(tx.vin[i].scriptSig, prevout.scriptPubKey, &tx.vin[i].scriptWitness, flags);
    }
    return nSigOps;
}

bool CheckTransaction(const CTransaction& tx, CValidationState &state, CTokensCache* tokenCache, bool fCheckDuplicateInputs, bool fMemPoolCheck, bool fCheckTokenDuplicate, bool fForceDuplicateCheck)
{
    // Basic checks that don't depend on any context
    if (tx.vin.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vin-empty");
    if (tx.vout.empty())
        return state.DoS(10, false, REJECT_INVALID, "bad-txns-vout-empty");
    // Size limits (this doesn't take the witness into account, as that hasn't been checked for malleability)
    if (::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * WITNESS_SCALE_FACTOR > GetMaxBlockWeight())
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-oversize");

    // Check for negative or overflow output values
    CAmount nValueOut = 0;
    for (const auto& txout : tx.vout)
    {
        if (txout.IsEmpty() && !tx.IsCoinBase() && !tx.IsCoinStake())
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-empty");

        if (txout.nValue < 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-negative");

        if (txout.nValue > MAX_MONEY)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-vout-toolarge");

        nValueOut += txout.nValue;
        if (!MoneyRange(nValueOut))
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-txouttotal-toolarge");

        /** TOKENS START */
        bool isToken = false;
        int nType;
        bool fIsOwner;
        if (txout.scriptPubKey.IsTokenScript(nType, fIsOwner))
            isToken = true;

        // Make sure that all token tx have a nValue of zero ALP
        if (isToken && txout.nValue != 0)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-token-tx-amount-isn't-zero");

        if (!AreTokensDeployed() && isToken && !fReindex)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-is-token-and-token-not-active");

        // Check for transfers that don't meet the tokens units only if the tokenCache is not null
        if (AreTokensDeployed() && isToken) {
            if (tokenCache) {
                // Get the transfer transaction data from the scriptPubKey
                if ( nType == TX_TRANSFER_TOKEN) {
                    CTokenTransfer transfer;
                    std::string address;
                    if (!TransferTokenFromScript(txout.scriptPubKey, transfer, address))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-token-bad-deserialize");

                    // Check token name validity and get type
                    TokenType tokenType;
                    if (!IsTokenNameValid(transfer.strName, tokenType)) {
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-token-name-invalid");
                    }

                    // If the transfer is an ownership token. Check to make sure that it is OWNER_TOKEN_AMOUNT
                    if (IsTokenNameAnOwner(transfer.strName)) {
                        if (transfer.nAmount != OWNER_TOKEN_AMOUNT)
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-owner-amount-was-not-1");
                    }

                    // If the transfer is a unique token. Check to make sure that it is UNIQUE_TOKEN_AMOUNT
                    if (tokenType == TokenType::UNIQUE) {
                        if (transfer.nAmount != UNIQUE_TOKEN_AMOUNT)
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-unique-amount-was-not-1");
                    }

                    if (tokenType == TokenType::MSGCHANNEL || tokenType == TokenType::VOTE) {
                        return state.DoS(100, false, REJECT_INVALID, "disabled-token-type");
                    }

                }
            }
        }
    }
    /** TOKENS END */

    if (fCheckDuplicateInputs) {
        std::set<COutPoint> vInOutPoints;
        for (const auto& txin : tx.vin)
        {
            if (!vInOutPoints.insert(txin.prevout).second)
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-duplicate");
        }
    }

    if (tx.IsCoinBase())
    {
        if (tx.vin[0].scriptSig.size() < 2 || tx.vin[0].scriptSig.size() > 100)
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-length");
    }
    else
    {
        for (const auto& txin : tx.vin)
            if (txin.prevout.IsNull())
                return state.DoS(10, false, REJECT_INVALID, "bad-txns-prevout-null");
    }

    /** TOKENS START */
    if (AreTokensDeployed()) {
        if (tokenCache) {
            if (tx.IsNewToken()) {

                /** Verify the reissue tokens data */
                std::string strError = "";
                if(!tx.VerifyNewToken(strError))
                    return state.DoS(100, false, REJECT_INVALID, strError);

                CNewToken token;
                std::string strAddress;
                if (!TokenFromTransaction(tx, token, strAddress))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-issue-token-from-transaction");

                // Validate the new tokens information
                if (!IsNewOwnerTxValid(tx, token.strName, strAddress, strError))
                    return state.DoS(100, false, REJECT_INVALID, strError);

                if (!token.IsValid(strError, *tokenCache, fMemPoolCheck, fCheckTokenDuplicate, fForceDuplicateCheck))
                    return state.DoS(100, error("%s: %s", __func__, strError), REJECT_INVALID, "bad-txns-issue-" + strError);

            } else if (tx.IsReissueToken()) {
                /** Verify the reissue tokens data */
                std::string strError;
                if (!tx.VerifyReissueToken(strError))
                    return state.DoS(100, false, REJECT_INVALID, strError);

                CReissueToken reissue;
                std::string strAddress;
                if (!ReissueTokenFromTransaction(tx, reissue, strAddress))
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-reissue-token");

                if (!reissue.IsValid(strError, *tokenCache, false)) {
                    return state.DoS(100, false, REJECT_INVALID, "bad-txns-reissue-" + strError);
                }

            } else if (tx.IsNewUniqueToken()) {

                /** Verify the unique tokens data */
                std::string strError = "";
                if (!tx.VerifyNewUniqueToken(strError)) {
                    return state.DoS(100, false, REJECT_INVALID, strError);
                }

                for (auto out : tx.vout)
                {
                    if (IsScriptNewUniqueToken(out.scriptPubKey))
                    {
                        CNewToken token;
                        std::string strAddress;
                        if (!TokenFromScript(out.scriptPubKey, token, strAddress))
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-check-transaction-issue-unique-token-serialization");

                        if (!token.IsValid(strError, *tokenCache, fMemPoolCheck, fCheckTokenDuplicate, fForceDuplicateCheck))
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-" + strError);
                    }
                }
            } else {
                // Fail if transaction contains any non-transfer token scripts and hasn't conformed to one of the
                // above transaction types.  Also fail if it contains OP_ALP_TOKEN opcode but wasn't a valid script.
                for (auto out : tx.vout) {
                    int nType;
                    bool _isOwner;
                    if (out.scriptPubKey.IsTokenScript(nType, _isOwner)) {
                        if (nType != TX_TRANSFER_TOKEN) {
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-bad-token-transaction");
                        }
                    } else {
                        if (out.scriptPubKey.Find(OP_ALP_TOKEN) > 0) {
                            return state.DoS(100, false, REJECT_INVALID, "bad-txns-bad-token-script");
                        }
                    }
                }
            }
        }
    }
    /** TOKENS END */

    return true;
}

bool Consensus::CheckTxInputs(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, CAmount& txfee)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missingorspent", false,
                         strprintf("%s: inputs missing/spent", __func__));
    }

    CAmount nValueIn = 0;
    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        // If prev is coinbase, check that it's matured
        if (coin.IsCoinBase() && nSpendHeight - coin.nHeight < CParams().GetConsensus().nCoinbaseMaturity) {
            return state.Invalid(false,
                REJECT_INVALID, "bad-txns-premature-spend-of-coinbase",
                strprintf("tried to spend coinbase at depth %d", nSpendHeight - coin.nHeight));
        }

        // If prev is coinstake, check that it's matured
        // Will be fixed after tokens deployed
        if (coin.IsCoinStake() && nSpendHeight - coin.nHeight < (coin.nHeight >= CParams().GetConsensus().nTokensDeploymentHeight ? CParams().GetConsensus().nStakeMaturity : CParams().GetConsensus().nCoinbaseMaturity)) {
            return state.Invalid(false,
                REJECT_INVALID, "bad-txns-premature-spend-of-coinstake",
                strprintf("tried to spend coinstake at depth %d, %d, %d", nSpendHeight, coin.nHeight, nSpendHeight - coin.nHeight));
        }

        // Check transaction timestamp
        if (coin.nTime > tx.nTime) {
            return state.DoS(100, error("CheckInputs() : transaction timestamp earlier than input transaction"),
                REJECT_INVALID, "bad-txns-time-earlier-than-input");
        }

        // Check for negative or overflow input values
        nValueIn += coin.out.nValue;
        if (!MoneyRange(coin.out.nValue) || !MoneyRange(nValueIn)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputvalues-outofrange");
        }
    }

    const CAmount value_out = tx.GetValueOut();
    CAmount txfee_aux = 0;

    if (!tx.IsCoinStake()) {
        txfee_aux = nValueIn - value_out;
    }

    if (!tx.IsCoinStake()) {
        if (nValueIn < value_out) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-in-belowout", false,
                strprintf("value in (%s) < value out (%s)", FormatMoney(nValueIn), FormatMoney(value_out)));
        }

        // Tally transaction fees
        if (!MoneyRange(txfee_aux)) {
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-fee-out-of-range");
        }
    }
    
    txfee = txfee_aux;
    return true;
}

//! Check to make sure that the inputs and outputs CAmount match exactly.
bool Consensus::CheckTxTokens(const CTransaction& tx, CValidationState& state, const CCoinsViewCache& inputs, int nSpendHeight, int64_t nSpendTime, std::vector<std::pair<std::string, uint256> >& vPairReissueTokens, const bool fRunningUnitTests)
{
    // are the actual inputs available?
    if (!inputs.HaveInputs(tx)) {
        return state.DoS(100, false, REJECT_INVALID, "bad-txns-inputs-missing-or-spent", false,
                         strprintf("%s: inputs missing/spent", __func__));
    }

    // Create map that stores the amount of an token transaction input. Used to verify no tokens are burned
    std::map<std::string, CAmount> totalInputs;

    for (unsigned int i = 0; i < tx.vin.size(); ++i) {
        const COutPoint &prevout = tx.vin[i].prevout;
        const Coin& coin = inputs.AccessCoin(prevout);
        assert(!coin.IsSpent());

        if (coin.IsToken()) {
            std::string strName;
            CAmount nAmount;
            uint32_t nTokenLockTime;

            if (!GetTokenInfoFromCoin(coin, strName, nAmount, nTokenLockTime))
                return state.DoS(100, false, REJECT_INVALID, "bad-txns-failed-to-get-token-from-script");

            // Add to the total value of tokens in the inputs
            if (totalInputs.count(strName))
                totalInputs.at(strName) += nAmount;
            else
                totalInputs.insert(make_pair(strName, nAmount));

            if ((int64_t)nTokenLockTime > ((int64_t)nTokenLockTime < LOCKTIME_THRESHOLD ? (int64_t)nSpendHeight : nSpendTime)) {
                std::string errorMsg = strprintf("Tried to spend token before %d", nTokenLockTime);
                return state.DoS(100, false,
                    REJECT_INVALID, "bad-tx-token-premature-spend-of-token " + errorMsg);
            }
        }
    }

    // Create map that stores the amount of an token transaction output. Used to verify no tokens are burned
    std::map<std::string, CAmount> totalOutputs;

    for (const auto& txout : tx.vout) {
        if (txout.scriptPubKey.IsTransferToken()) {
            CTokenTransfer transfer;
            std::string address;
            if (!TransferTokenFromScript(txout.scriptPubKey, transfer, address))
                return state.DoS(100, false, REJECT_INVALID, "bad-tx-token-transfer-bad-deserialize");

            // Add to the total value of tokens in the outputs
            if (totalOutputs.count(transfer.strName))
                totalOutputs.at(transfer.strName) += transfer.nAmount;
            else
                totalOutputs.insert(make_pair(transfer.strName, transfer.nAmount));

            auto currentActiveTokenCache = GetCurrentTokenCache();
            if (!fRunningUnitTests) {
                if (IsTokenNameAnOwner(transfer.strName)) {
                    if (transfer.nAmount != OWNER_TOKEN_AMOUNT)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-owner-amount-was-not-1");
                } else {
                    // For all other types of tokens, make sure they are sending the right type of units
                    CNewToken token;
                    if (!currentActiveTokenCache->GetTokenMetaDataIfExists(transfer.strName, token))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-token-not-exist");

                    if (token.strName != transfer.strName)
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-token-database-corrupted");

                    if (!CheckAmountWithUnits(transfer.nAmount, token.units))
                        return state.DoS(100, false, REJECT_INVALID, "bad-txns-transfer-token-amount-not-match-units");
                }
            }
        } else if (txout.scriptPubKey.IsReissueToken()) {
            CReissueToken reissue;
            std::string address;
            if (!ReissueTokenFromScript(txout.scriptPubKey, reissue, address))
                return state.DoS(100, false, REJECT_INVALID, "bad-tx-token-reissue-bad-deserialize");

            if (!fRunningUnitTests) {
                auto currentActiveTokenCache = GetCurrentTokenCache();
                std::string strError;
                if (!reissue.IsValid(strError, *currentActiveTokenCache)) {
                    return state.DoS(100, false, REJECT_INVALID,
                                     "bad-txns" + strError);
                }
            }

            if (mapReissuedTokens.count(reissue.strName)) {
                if (mapReissuedTokens.at(reissue.strName) != tx.GetHash())
                    return state.DoS(100, false, REJECT_INVALID, "bad-tx-reissue-chaining-not-allowed");
            } else {
                vPairReissueTokens.emplace_back(std::make_pair(reissue.strName, tx.GetHash()));
            }
        }
    }

    for (const auto& outValue : totalOutputs) {
        if (!totalInputs.count(outValue.first)) {
            std::string errorMsg;
            errorMsg = strprintf("Bad Transaction - Trying to create outpoint for token that you don't have: %s", outValue.first);
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-inputs-outputs-mismatch " + errorMsg);
        }

        if (totalInputs.at(outValue.first) != outValue.second) {
            std::string errorMsg;
            errorMsg = strprintf("Bad Transaction - Tokens would be burnt %s", outValue.first);
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-inputs-outputs-mismatch " + errorMsg);
        }
    }

    // Check the input size and the output size
    if (totalOutputs.size() != totalInputs.size()) {
        return state.DoS(100, false, REJECT_INVALID, "bad-tx-token-inputs-size-does-not-match-outputs-size");
    }

    return true;
}
