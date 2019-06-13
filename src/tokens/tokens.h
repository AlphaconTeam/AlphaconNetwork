// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ALPHACONCOIN_TOKEN_PROTOCOL_H
#define ALPHACONCOIN_TOKEN_PROTOCOL_H

#include "amount.h"
#include "tinyformat.h"
#include "tokentypes.h"

#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include <list>

#define ALP_A 97
#define ALP_L 108
#define ALP_P 112
#define ALP_Q 113
#define ALP_T 116
#define ALP_O 111

#define DEFAULT_UNITS 0
#define DEFAULT_REISSUABLE 1
#define DEFAULT_HAS_IPFS 0
#define DEFAULT_IPFS ""
#define MIN_TOKEN_LENGTH 3
#define MAX_TOKEN_LENGTH 32
#define OWNER_TAG "!"
#define OWNER_LENGTH 1
#define OWNER_UNITS 0
#define OWNER_TOKEN_AMOUNT 1 * COIN
#define UNIQUE_TOKEN_AMOUNT 1 * COIN
#define UNIQUE_TOKEN_UNITS 0
#define UNIQUE_TOKENS_REISSUABLE 0

#define TOKEN_TRANSFER_STRING "transfer_token"
#define TOKEN_NEW_STRING "new_token"
#define TOKEN_REISSUE_STRING "reissue_token"

class CScript;
class CDataStream;
class CTransaction;
class CTxOut;
class Coin;
class CWallet;
class CReserveKey;
class CWalletTx;
struct CTokenOutputEntry;
class CCoinControl;
struct CBlockTokenUndo;
class COutput;

// 2500 * 82 Bytes == 205 KB (kilobytes) of memory
#define MAX_CACHE_TOKENS_SIZE 2500

// Create map that store that state of current reissued transaction that the mempool as accepted.
// If an token name is in this map, any other reissue transactions wont be accepted into the mempool
extern std::map<uint256, std::string> mapReissuedTx;
extern std::map<std::string, uint256> mapReissuedTokens;

class CTokens {
public:
    std::map<std::pair<std::string, std::string>, CAmount> mapTokensAddressAmount; // pair < Token Name , Address > -> Quantity of tokens in the address

    // Dirty, Gets wiped once flushed to database
    std::map<std::string, CNewToken> mapReissuedTokenData; // Token Name -> New Token Data

    CTokens(const CTokens& tokens) {
        this->mapTokensAddressAmount = tokens.mapTokensAddressAmount;
        this->mapReissuedTokenData = tokens.mapReissuedTokenData;
    }

    CTokens& operator=(const CTokens& other) {
        mapTokensAddressAmount = other.mapTokensAddressAmount;
        mapReissuedTokenData = other.mapReissuedTokenData;
        return *this;
    }

    CTokens() {
        SetNull();
    }

    void SetNull() {
        mapTokensAddressAmount.clear();
        mapReissuedTokenData.clear();
    }
};

class CTokensCache : public CTokens
{
private:
    bool AddBackSpentToken(const Coin& coin, const std::string& tokenName, const std::string& address, const CAmount& nAmount, const COutPoint& out);
    void AddToTokenBalance(const std::string& strName, const std::string& address, const CAmount& nAmount);
    bool UndoTransfer(const CTokenTransfer& transfer, const std::string& address, const COutPoint& outToRemove);
public :
    //! These are memory only containers that show dirty entries that will be databased when flushed
    std::vector<CTokenCacheUndoTokenAmount> vUndoTokenAmount;
    std::vector<CTokenCacheSpendToken> vSpentTokens;

    // New Tokens Caches
    std::set<CTokenCacheNewToken> setNewTokensToRemove;
    std::set<CTokenCacheNewToken> setNewTokensToAdd;

    // New Reissue Caches
    std::set<CTokenCacheReissueToken> setNewReissueToRemove;
    std::set<CTokenCacheReissueToken> setNewReissueToAdd;

    // Ownership Tokens Caches
    std::set<CTokenCacheNewOwner> setNewOwnerTokensToAdd;
    std::set<CTokenCacheNewOwner> setNewOwnerTokensToRemove;

    // Transfer Tokens Caches
    std::set<CTokenCacheNewTransfer> setNewTransferTokensToAdd;
    std::set<CTokenCacheNewTransfer> setNewTransferTokensToRemove;

    CTokensCache() : CTokens()
    {
        SetNull();
        ClearDirtyCache();
    }

    CTokensCache(const CTokensCache& cache) : CTokens(cache)
    {
        // Copy dirty cache also
        this->vSpentTokens = cache.vSpentTokens;
        this->vUndoTokenAmount = cache.vUndoTokenAmount;

        // Transfer Caches
        this->setNewTransferTokensToAdd = cache.setNewTransferTokensToAdd;
        this->setNewTransferTokensToRemove = cache.setNewTransferTokensToRemove;

        // Issue Caches
        this->setNewTokensToRemove = cache.setNewTokensToRemove;
        this->setNewTokensToAdd = cache.setNewTokensToAdd;

        // Reissue Caches
        this->setNewReissueToRemove = cache.setNewReissueToRemove;
        this->setNewReissueToAdd = cache.setNewReissueToAdd;

        // Owner Caches
        this->setNewOwnerTokensToAdd = cache.setNewOwnerTokensToAdd;
        this->setNewOwnerTokensToRemove = cache.setNewOwnerTokensToRemove;
    }

    CTokensCache& operator=(const CTokensCache& cache)
    {
        this->mapTokensAddressAmount = cache.mapTokensAddressAmount;
        this->mapReissuedTokenData = cache.mapReissuedTokenData;

        // Copy dirty cache also
        this->vSpentTokens = cache.vSpentTokens;
        this->vUndoTokenAmount = cache.vUndoTokenAmount;

        // Transfer Caches
        this->setNewTransferTokensToAdd = cache.setNewTransferTokensToAdd;
        this->setNewTransferTokensToRemove = cache.setNewTransferTokensToRemove;

        // Issue Caches
        this->setNewTokensToRemove = cache.setNewTokensToRemove;
        this->setNewTokensToAdd = cache.setNewTokensToAdd;

        // Reissue Caches
        this->setNewReissueToRemove = cache.setNewReissueToRemove;
        this->setNewReissueToAdd = cache.setNewReissueToAdd;

        // Owner Caches
        this->setNewOwnerTokensToAdd = cache.setNewOwnerTokensToAdd;
        this->setNewOwnerTokensToRemove = cache.setNewOwnerTokensToRemove;

        return *this;
    }

    // Cache only undo functions
    bool RemoveNewToken(const CNewToken& token, const std::string address);
    bool RemoveTransfer(const CTokenTransfer& transfer, const std::string& address, const COutPoint& out);
    bool RemoveOwnerToken(const std::string& tokensName, const std::string address);
    bool RemoveReissueToken(const CReissueToken& reissue, const std::string address, const COutPoint& out, const std::vector<std::pair<std::string, CBlockTokenUndo> >& vUndoIPFS);
    bool UndoTokenCoin(const Coin& coin, const COutPoint& out);

    // Cache only add token functions
    bool AddNewToken(const CNewToken& token, const std::string address, const int& nHeight, const uint256& blockHash);
    bool AddTransferToken(const CTokenTransfer& transferToken, const std::string& address, const COutPoint& out, const CTxOut& txOut);
    bool AddOwnerToken(const std::string& tokensName, const std::string address);
    bool AddReissueToken(const CReissueToken& reissue, const std::string address, const COutPoint& out);

    // Cache only validation functions
    bool TrySpendCoin(const COutPoint& out, const CTxOut& coin);

    // Help functions
    bool ContainsToken(const CNewToken& token);
    bool ContainsToken(const std::string& tokenName);

    bool CheckIfTokenExists(const std::string& name, bool fForceDuplicateCheck = true);
    bool GetTokenMetaDataIfExists(const std::string &name, CNewToken &token, int& nHeight, uint256& blockHash);
    bool GetTokenMetaDataIfExists(const std::string &name, CNewToken &token);

    //! Calculate the size of the CTokens (in bytes)
    size_t DynamicMemoryUsage() const;

    //! Get the size of the none databased cache
    size_t GetCacheSize() const;
    size_t GetCacheSizeV2() const;

    //! Flush all new cache entries into the ptokens global cache
    bool Flush();

    //! Write token cache data to database
    bool DumpCacheToDatabase();

    void ClearDirtyCache() {

        vUndoTokenAmount.clear();
        vSpentTokens.clear();

        setNewTokensToRemove.clear();
        setNewTokensToAdd.clear();

        setNewReissueToAdd.clear();
        setNewReissueToRemove.clear();

        setNewTransferTokensToAdd.clear();
        setNewTransferTokensToRemove.clear();

        setNewOwnerTokensToAdd.clear();
        setNewOwnerTokensToRemove.clear();

        mapReissuedTokenData.clear();
        mapTokensAddressAmount.clear();
    }

   std::string CacheToString() const {

       return strprintf(
               "vNewTokensToRemove size : %d, vNewTokensToAdd size : %d, vNewTransfer size : %d, vSpentTokens : %d\n",
               setNewTokensToRemove.size(), setNewTokensToAdd.size(), setNewTransferTokensToAdd.size(),
               vSpentTokens.size());
   }
};

// Functions to be used to get access to the current burn amount required for specific token issuance transactions
CAmount GetIssueTokenBurnAmount();
CAmount GetReissueTokenBurnAmount();
CAmount GetIssueSubTokenBurnAmount();
CAmount GetIssueUniqueTokenBurnAmount();
CAmount GetBurnAmount(const TokenType type);
CAmount GetBurnAmount(const int nType);
std::string GetBurnAddress(const TokenType type);
std::string GetBurnAddress(const int nType);

void GetTxOutTokenTypes(const std::vector<CTxOut>& vout, int& issues, int& reissues, int& transfers, int& owners);

bool IsTokenNameValid(const std::string& name);
bool IsTokenNameValid(const std::string& name, TokenType& tokenType);
bool IsTokenNameValid(const std::string& name, TokenType& tokenType, std::string& error);
bool IsUniqueTagValid(const std::string& tag);
bool IsTokenNameAnOwner(const std::string& name);
std::string GetParentName(const std::string& name); // Gets the parent name of a subtoken TEST/TESTSUB would return TEST
std::string GetUniqueTokenName(const std::string& parent, const std::string& tag);

bool IsTypeCheckNameValid(const TokenType type, const std::string& name, std::string& error);

bool IsTokenUnitsValid(const CAmount& units);

bool TokenFromTransaction(const CTransaction& tx, CNewToken& token, std::string& strAddress);
bool OwnerFromTransaction(const CTransaction& tx, std::string& ownerName, std::string& strAddress);
bool ReissueTokenFromTransaction(const CTransaction& tx, CReissueToken& reissue, std::string& strAddress);
bool UniqueTokenFromTransaction(const CTransaction& tx, CNewToken& token, std::string& strAddress);

bool TransferTokenFromScript(const CScript& scriptPubKey, CTokenTransfer& tokenTransfer, std::string& strAddress);
bool TokenFromScript(const CScript& scriptPubKey, CNewToken& token, std::string& strAddress);
bool OwnerTokenFromScript(const CScript& scriptPubKey, std::string& tokenName, std::string& strAddress);
bool ReissueTokenFromScript(const CScript& scriptPubKey, CReissueToken& reissue, std::string& strAddress);

bool CheckIssueBurnTx(const CTxOut& txOut, const TokenType& type, const int numberIssued);
bool CheckIssueBurnTx(const CTxOut& txOut, const TokenType& type);
bool CheckReissueBurnTx(const CTxOut& txOut);

bool CheckIssueDataTx(const CTxOut& txOut);
bool CheckOwnerDataTx(const CTxOut& txOut);
bool CheckReissueDataTx(const CTxOut& txOut);
bool CheckTransferOwnerTx(const CTxOut& txOut);

bool CheckEncodedIPFS(const std::string& hash, std::string& strError);

bool CheckAmountWithUnits(const CAmount& nAmount, const int8_t nUnits);

bool IsScriptNewToken(const CScript& scriptPubKey, int& nStartingIndex);
bool IsScriptNewUniqueToken(const CScript& scriptPubKey, int& nStartingIndex);
bool IsScriptOwnerToken(const CScript& scriptPubKey, int& nStartingIndex);
bool IsScriptReissueToken(const CScript& scriptPubKey, int& nStartingIndex);
bool IsScriptTransferToken(const CScript& scriptPubKey, int& nStartingIndex);
bool IsScriptNewToken(const CScript& scriptPubKey);
bool IsScriptNewUniqueToken(const CScript& scriptPubKey);
bool IsScriptOwnerToken(const CScript& scriptPubKey);
bool IsScriptReissueToken(const CScript& scriptPubKey);
bool IsScriptTransferToken(const CScript& scriptPubKey);

bool IsNewOwnerTxValid(const CTransaction& tx, const std::string& tokenName, const std::string& address, std::string& errorMsg);

void GetAllAdministrativeTokens(CWallet *pwallet, std::vector<std::string> &names, int nMinConf = 1);
void GetAllMyTokens(CWallet* pwallet, std::vector<std::string>& names, int nMinConf = 1, bool fIncludeAdministrator = false, bool fOnlyAdministrator = false);

bool GetTokenInfoFromCoin(const Coin& coin, std::string& strName, CAmount& nAmount, uint32_t& nTokenLockTime);
bool GetTokenInfoFromScript(const CScript& scriptPubKey, std::string& strName, CAmount& nAmount, uint32_t& nTokenLockTime);

bool GetTokenData(const CScript& script, CTokenOutputEntry& data);

bool GetBestTokenAddressAmount(CTokensCache& cache, const std::string& tokenName, const std::string& address);

bool GetAllMyTokenBalances(std::map<std::string, std::vector<COutput> >& outputs, std::map<std::string, CAmount>& amounts, const std::string& prefix = "");
bool GetAllMyLockedTokenBalances(std::map<std::string, std::vector<COutput> >& outputs, std::map<std::string, CAmount>& amounts, const std::string& prefix = "");

/** Verifies that this wallet owns the give token */
bool VerifyWalletHasToken(const std::string& token_name, std::pair<int, std::string>& pairError);

std::string DecodeIPFS(std::string encoded);
std::string EncodeIPFS(std::string decoded);

bool CreateTokenTransaction(CWallet* pwallet, CCoinControl& coinControl, const CNewToken& token, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired);
bool CreateTokenTransaction(CWallet* pwallet, CCoinControl& coinControl, const std::vector<CNewToken> tokens, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired);
bool CreateReissueTokenTransaction(CWallet* pwallet, CCoinControl& coinControl, const CReissueToken& token, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired);
bool CreateTransferTokenTransaction(CWallet* pwallet, const CCoinControl& coinControl, const std::vector< std::pair<CTokenTransfer, std::string> >vTransfers, const std::string& changeAddress, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired);
bool SendTokenTransaction(CWallet* pwallet, CWalletTx& transaction, CReserveKey& reserveKey, std::pair<int, std::string>& error, std::string& txid);

/** Helper method for extracting address bytes, token name and amount from an token script */
bool ParseTokenScript(CScript scriptPubKey, uint160 &hashBytes, std::string &tokenName, CAmount &tokenAmount);
#endif //ALPHACONCOIN_TOKEN_PROTOCOL_H
