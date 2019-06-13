// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <regex>
#include <script/script.h>
#include <version.h>
#include <streams.h>
#include <primitives/transaction.h>
#include <iostream>
#include <script/standard.h>
#include <util.h>
#include <chainparams.h>
#include <base58.h>
#include <validation.h>
#include <txmempool.h>
#include <tinyformat.h>
#include <wallet/wallet.h>
#include <boost/algorithm/string.hpp>
#include <consensus/validation.h>
#include <rpc/protocol.h>
#include <net.h>
#include "tokens.h"
#include "tokendb.h"
#include "tokentypes.h"
#include "protocol.h"
#include "wallet/coincontrol.h"
#include "utilmoneystr.h"
#include "coins.h"
#include "wallet/wallet.h"

std::map<uint256, std::string> mapReissuedTx;
std::map<std::string, uint256> mapReissuedTokens;

// excluding owner tag ('!')
static const auto MAX_NAME_LENGTH = 31;
static const auto MAX_CHANNEL_NAME_LENGTH = 12;

// min lengths are expressed by quantifiers
static const std::regex ROOT_NAME_CHARACTERS("^[A-Z0-9._]{3,}$");
static const std::regex SUB_NAME_CHARACTERS("^[A-Z0-9._]+$");
static const std::regex UNIQUE_TAG_CHARACTERS("^[-A-Za-z0-9@$%&*()[\\]{}_.?:]+$");
static const std::regex CHANNEL_TAG_CHARACTERS("^[A-Z0-9._]+$");
static const std::regex VOTE_TAG_CHARACTERS("^[A-Z0-9._]+$");

static const std::regex DOUBLE_PUNCTUATION("^.*[._]{2,}.*$");
static const std::regex LEADING_PUNCTUATION("^[._].*$");
static const std::regex TRAILING_PUNCTUATION("^.*[._]$");

static const std::string SUB_NAME_DELIMITER = "/";
static const std::string UNIQUE_TAG_DELIMITER = "#";
static const std::string CHANNEL_TAG_DELIMITER = "~";
static const std::string VOTE_TAG_DELIMITER = "^";

static const std::regex UNIQUE_INDICATOR(R"(^[^^~#!]+#[^~#!\/]+$)");
static const std::regex CHANNEL_INDICATOR(R"(^[^^~#!]+~[^~#!\/]+$)");
static const std::regex OWNER_INDICATOR(R"(^[^^~#!]+!$)");
static const std::regex VOTE_INDICATOR(R"(^[^^~#!]+\^[^~#!\/]+$)");

static const std::regex PROTECTED_NAMES("^ALP$|^ALPHACON$|^ALPCOIN$|^ALPHACOIN$|^ALPHACHAIN$");

bool IsRootNameValid(const std::string& name)
{
    return std::regex_match(name, ROOT_NAME_CHARACTERS)
        && !std::regex_match(name, DOUBLE_PUNCTUATION)
        && !std::regex_match(name, LEADING_PUNCTUATION)
        && !std::regex_match(name, TRAILING_PUNCTUATION)
        && !std::regex_match(name, PROTECTED_NAMES);
}

bool IsSubNameValid(const std::string& name)
{
    return std::regex_match(name, SUB_NAME_CHARACTERS)
        && !std::regex_match(name, DOUBLE_PUNCTUATION)
        && !std::regex_match(name, LEADING_PUNCTUATION)
        && !std::regex_match(name, TRAILING_PUNCTUATION);
}

bool IsUniqueTagValid(const std::string& tag)
{
    return std::regex_match(tag, UNIQUE_TAG_CHARACTERS);
}

bool IsVoteTagValid(const std::string& tag)
{
    return std::regex_match(tag, VOTE_TAG_CHARACTERS);
}

bool IsChannelTagValid(const std::string& tag)
{
    return std::regex_match(tag, CHANNEL_TAG_CHARACTERS)
        && !std::regex_match(tag, DOUBLE_PUNCTUATION)
        && !std::regex_match(tag, LEADING_PUNCTUATION)
        && !std::regex_match(tag, TRAILING_PUNCTUATION);
}

bool IsNameValidBeforeTag(const std::string& name)
{
    std::vector<std::string> parts;
    boost::split(parts, name, boost::is_any_of(SUB_NAME_DELIMITER));

    if (!IsRootNameValid(parts.front())) return false;

    if (parts.size() > 1)
    {
        for (unsigned long i = 1; i < parts.size(); i++)
        {
            if (!IsSubNameValid(parts[i])) return false;
        }
    }

    return true;
}

bool IsTokenNameASubtoken(const std::string& name)
{
    std::vector<std::string> parts;
    boost::split(parts, name, boost::is_any_of(SUB_NAME_DELIMITER));

    if (!IsRootNameValid(parts.front())) return false;

    return parts.size() > 1;
}

bool IsTokenNameValid(const std::string& name, TokenType& tokenType, std::string& error)
{
    tokenType = TokenType::INVALID;
    if (std::regex_match(name, UNIQUE_INDICATOR))
    {
        bool ret = IsTypeCheckNameValid(TokenType::UNIQUE, name, error);
        if (ret)
            tokenType = TokenType::UNIQUE;

        return ret;
    }
    else if (std::regex_match(name, CHANNEL_INDICATOR))
    {
        bool ret = IsTypeCheckNameValid(TokenType::MSGCHANNEL, name, error);
        if (ret)
            tokenType = TokenType::MSGCHANNEL;

        return ret;
    }
    else if (std::regex_match(name, OWNER_INDICATOR))
    {
        bool ret = IsTypeCheckNameValid(TokenType::OWNER, name, error);
        if (ret)
            tokenType = TokenType::OWNER;

        return ret;
    }
    else if (std::regex_match(name, VOTE_INDICATOR))
    {
        bool ret = IsTypeCheckNameValid(TokenType::VOTE, name, error);
        if (ret)
            tokenType = TokenType::VOTE;

        return ret;
    }
    else
    {
        auto type = IsTokenNameASubtoken(name) ? TokenType::SUB : TokenType::ROOT;
        bool ret = IsTypeCheckNameValid(type, name, error);
        if (ret)
            tokenType = type;

        return ret;
    }
}

bool IsTokenNameValid(const std::string& name)
{
    TokenType _tokenType;
    std::string _error;
    return IsTokenNameValid(name, _tokenType, _error);
}

bool IsTokenNameValid(const std::string& name, TokenType& tokenType)
{
    std::string _error;
    return IsTokenNameValid(name, tokenType, _error);
}

bool IsTokenNameAnOwner(const std::string& name)
{
    return IsTokenNameValid(name) && std::regex_match(name, OWNER_INDICATOR);
}

// TODO get the string translated below
bool IsTypeCheckNameValid(const TokenType type, const std::string& name, std::string& error)
{
    if (type == TokenType::UNIQUE) {
        if (name.size() > MAX_NAME_LENGTH) { error = "Name is greater than max length of " + std::to_string(MAX_NAME_LENGTH); return false; }
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(UNIQUE_TAG_DELIMITER));
        bool valid = IsNameValidBeforeTag(parts.front()) && IsUniqueTagValid(parts.back());
        if (!valid) { error = "Unique name contains invalid characters (Valid characters are: A-Z a-z 0-9 @ $ % & * ( ) [ ] { } _ . ? : -)";  return false; }
        return true;
    } else if (type == TokenType::MSGCHANNEL) {
        if (name.size() > MAX_NAME_LENGTH) { error = "Name is greater than max length of " + std::to_string(MAX_NAME_LENGTH); return false; }
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(CHANNEL_TAG_DELIMITER));
        bool valid = IsNameValidBeforeTag(parts.front()) && IsChannelTagValid(parts.back());
        if (parts.back().size() > MAX_CHANNEL_NAME_LENGTH) { error = "Channel name is greater than max length of " + std::to_string(MAX_CHANNEL_NAME_LENGTH); return false; }
        if (!valid) { error = "Message Channel name contains invalid characters (Valid characters are: A-Z 0-9 _ .) (special characters can't be the first or last characters)";  return false; }
        return true;
    } else if (type == TokenType::OWNER) {
        if (name.size() > MAX_NAME_LENGTH) { error = "Name is greater than max length of " + std::to_string(MAX_NAME_LENGTH); return false; }
        bool valid = IsNameValidBeforeTag(name.substr(0, name.size() - 1));
        if (!valid) { error = "Owner name contains invalid characters (Valid characters are: A-Z 0-9 _ .) (special characters can't be the first or last characters)";  return false; }
        return true;
    } else if (type == TokenType::VOTE) {
        if (name.size() > MAX_NAME_LENGTH) { error = "Name is greater than max length of " + std::to_string(MAX_NAME_LENGTH); return false; }
        std::vector<std::string> parts;
        boost::split(parts, name, boost::is_any_of(VOTE_TAG_DELIMITER));
        bool valid = IsNameValidBeforeTag(parts.front()) && IsVoteTagValid(parts.back());
        if (!valid) { error = "Vote name contains invalid characters (Valid characters are: A-Z 0-9 _ .) (special characters can't be the first or last characters)";  return false; }
        return true;
    } else {
        if (name.size() > MAX_NAME_LENGTH - 1) { error = "Name is greater than max length of " + std::to_string(MAX_NAME_LENGTH - 1); return false; }  //Tokens and sub-tokens need to leave one extra char for OWNER indicator
        if (!IsTokenNameASubtoken(name) && name.size() < MIN_TOKEN_LENGTH) { error = "Name must be contain " + std::to_string(MIN_TOKEN_LENGTH) + " characters"; return false; }
        bool valid = IsNameValidBeforeTag(name);
        if (!valid && IsTokenNameASubtoken(name) && name.size() < 3) { error = "Name must have at least 3 characters (Valid characters are: A-Z 0-9 _ .)";  return false; }
        if (!valid) { error = "Name contains invalid characters (Valid characters are: A-Z 0-9 _ .) (special characters can't be the first or last characters)";  return false; }
        return true;
    }
}

std::string GetParentName(const std::string& name)
{
    TokenType type;
    if (!IsTokenNameValid(name, type))
        return "";

    auto index = std::string::npos;
    if (type == TokenType::SUB) {
        index = name.find_last_of(SUB_NAME_DELIMITER);
    } else if (type == TokenType::UNIQUE) {
        index = name.find_last_of(UNIQUE_TAG_DELIMITER);
    } else if (type == TokenType::MSGCHANNEL) {
        index = name.find_last_of(CHANNEL_TAG_DELIMITER);
    } else if (type == TokenType::VOTE) {
        index = name.find_last_of(VOTE_TAG_DELIMITER);
    } else if (type == TokenType::ROOT)
        return name;

    if (std::string::npos != index)
    {
        return name.substr(0, index);
    }

    return name;
}

std::string GetUniqueTokenName(const std::string& parent, const std::string& tag)
{
    if (!IsRootNameValid(parent))
        return "";

    if (!IsUniqueTagValid(tag))
        return "";

    return parent + "#" + tag;
}

bool CNewToken::IsNull() const
{
    return strName == "";
}

bool CNewToken::IsValid(std::string& strError, CTokensCache& tokenCache, bool fCheckMempool, bool fCheckDuplicateInputs, bool fForceDuplicateCheck) const
{
    strError = "";

    // Check our current ptokens to see if the token has been created yet
    if (fCheckDuplicateInputs) {
        if (tokenCache.CheckIfTokenExists(this->strName, fForceDuplicateCheck)) {
            strError = std::string(_("Invalid parameter: token_name '")) + strName + std::string(_("' has already been used"));
            return false;
        }
    }

    if (fCheckMempool) {
        if (mempool.mapTokenToHash.count(strName)) {
            strError = _("Token with this name is already in the mempool");
            return false;
        }
    }

    TokenType tokenType;
    if (!IsTokenNameValid(std::string(strName), tokenType)) {
        strError = _("Invalid parameter: token_name must only consist of valid characters and have a size between 3 and 30 characters. See help for more details.");
        return false;
    }

    if (tokenType == TokenType::UNIQUE) {
        if (units != UNIQUE_TOKEN_UNITS) {
            strError = _("Invalid parameter: units must be ") + std::to_string(UNIQUE_TOKEN_UNITS / COIN);
            return false;
        }
        if (nAmount != UNIQUE_TOKEN_AMOUNT) {
            strError = _("Invalid parameter: amount must be ") + std::to_string(UNIQUE_TOKEN_AMOUNT);
            return false;
        }
        if (nReissuable != 0) {
            strError = _("Invalid parameter: reissuable must be 0");
            return false;
        }
    }

    if (IsTokenNameAnOwner(std::string(strName))) {
        strError = _("Invalid parameters: token_name can't have a '!' at the end of it. See help for more details.");
        return false;
    }

    if (nAmount <= 0) {
        strError = _("Invalid parameter: token amount can't be equal to or less than zero.");
        return false;
    }

    if (nAmount > MAX_MONEY_TOKENS) {
        strError = _("Invalid parameter: token amount greater than max money: ") + std::to_string(MAX_MONEY_TOKENS / COIN);
        return false;
    }

    if (units < 0 || units > 8) {
        strError = _("Invalid parameter: units must be between 0-8.");
        return false;
    }

    if (!CheckAmountWithUnits(nAmount, units)) {
        strError = _("Invalid parameter: amount must be divisible by the smaller unit assigned to the token");
        return false;
    }

    if (nReissuable != 0 && nReissuable != 1) {
        strError = _("Invalid parameter: reissuable must be 0 or 1");
        return false;
    }

    if (nHasIPFS != 0) {
        strError = _("Invalid parameter: this feature is disabled.");
        return false;
    }

    if (strIPFSHash != "") {
        strError = _("Invalid parameter: this feature is disabled.");
        return false;
    }

    return true;
}

CNewToken::CNewToken(const CNewToken& token)
{
    this->strName = token.strName;
    this->nAmount = token.nAmount;
    this->units = token.units;
    this->nHasIPFS = token.nHasIPFS;
    this->nReissuable = token.nReissuable;
    this->strIPFSHash = token.strIPFSHash;
}

CNewToken& CNewToken::operator=(const CNewToken& token)
{
    this->strName = token.strName;
    this->nAmount = token.nAmount;
    this->units = token.units;
    this->nHasIPFS = token.nHasIPFS;
    this->nReissuable = token.nReissuable;
    this->strIPFSHash = token.strIPFSHash;
    return *this;
}

std::string CNewToken::ToString()
{
    std::stringstream ss;
    ss << "Printing an token" << "\n";
    ss << "name : " << strName << "\n";
    ss << "amount : " << nAmount << "\n";
    ss << "units : " << std::to_string(units) << "\n";
    ss << "reissuable : " << std::to_string(nReissuable) << "\n";

    return ss.str();
}

CNewToken::CNewToken(const std::string& strName, const CAmount& nAmount, const int& units, const int& nReissuable, const int& nHasIPFS, const std::string& strIPFSHash)
{
    this->SetNull();
    this->strName = strName;
    this->nAmount = nAmount;
    this->units = int8_t(units);
    this->nReissuable = int8_t(nReissuable);
    this->nHasIPFS = int8_t(nHasIPFS);
    this->strIPFSHash = strIPFSHash;
}
CNewToken::CNewToken(const std::string& strName, const CAmount& nAmount)
{
    this->SetNull();
    this->strName = strName;
    this->nAmount = nAmount;
    this->units = int8_t(DEFAULT_UNITS);
    this->nReissuable = int8_t(DEFAULT_REISSUABLE);
    this->nHasIPFS = int8_t(DEFAULT_HAS_IPFS);
    this->strIPFSHash = DEFAULT_IPFS;
}

CDatabasedTokenData::CDatabasedTokenData(const CNewToken& token, const int& nHeight, const uint256& blockHash)
{
    this->SetNull();
    this->token = token;
    this->nHeight = nHeight;
    this->blockHash = blockHash;
}

CDatabasedTokenData::CDatabasedTokenData()
{
    this->SetNull();
}

/**
 * Constructs a CScript that carries the token name and quantity and adds to to the end of the given script
 * @param dest - The destination that the token will belong to
 * @param script - This script needs to be a pay to address script
 */
void CNewToken::ConstructTransaction(CScript& script) const
{
    CDataStream ssToken(SER_NETWORK, PROTOCOL_VERSION);
    ssToken << *this;

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(ALP_A); // a
    vchMessage.push_back(ALP_L); // l
    vchMessage.push_back(ALP_P); // p
    vchMessage.push_back(ALP_Q); // q

    vchMessage.insert(vchMessage.end(), ssToken.begin(), ssToken.end());
    script << OP_ALP_TOKEN << ToByteVector(vchMessage) << OP_DROP;
}

void CNewToken::ConstructOwnerTransaction(CScript& script) const
{
    CDataStream ssOwner(SER_NETWORK, PROTOCOL_VERSION);
    ssOwner << std::string(this->strName + OWNER_TAG);

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(ALP_A); // a
    vchMessage.push_back(ALP_L); // l
    vchMessage.push_back(ALP_P); // p
    vchMessage.push_back(ALP_O); // o

    vchMessage.insert(vchMessage.end(), ssOwner.begin(), ssOwner.end());
    script << OP_ALP_TOKEN << ToByteVector(vchMessage) << OP_DROP;
}

bool TokenFromTransaction(const CTransaction& tx, CNewToken& token, std::string& strAddress)
{
    // Check to see if the transaction is an new token issue tx
    if (!tx.IsNewToken())
        return false;

    // Get the scriptPubKey from the last tx in vout
    CScript scriptPubKey = tx.vout[tx.vout.size() - 1].scriptPubKey;

    return TokenFromScript(scriptPubKey, token, strAddress);
}

bool ReissueTokenFromTransaction(const CTransaction& tx, CReissueToken& reissue, std::string& strAddress)
{
    // Check to see if the transaction is a reissue tx
    if (!tx.IsReissueToken())
        return false;

    // Get the scriptPubKey from the last tx in vout
    CScript scriptPubKey = tx.vout[tx.vout.size() - 1].scriptPubKey;

    return ReissueTokenFromScript(scriptPubKey, reissue, strAddress);
}

bool UniqueTokenFromTransaction(const CTransaction& tx, CNewToken& token, std::string& strAddress)
{
    // Check to see if the transaction is an new token issue tx
    if (!tx.IsNewUniqueToken())
        return false;

    // Get the scriptPubKey from the last tx in vout
    CScript scriptPubKey = tx.vout[tx.vout.size() - 1].scriptPubKey;

    return TokenFromScript(scriptPubKey, token, strAddress);
}

bool IsNewOwnerTxValid(const CTransaction& tx, const std::string& tokenName, const std::string& address, std::string& errorMsg)
{
    // TODO when ready to ship. Put the owner validation code in own method if needed
    std::string ownerName;
    std::string ownerAddress;
    if (!OwnerFromTransaction(tx, ownerName, ownerAddress)) {
        errorMsg = "bad-txns-bad-owner";
        return false;
    }

    int size = ownerName.size();

    if (ownerAddress != address) {
        errorMsg = "bad-txns-owner-address-mismatch";
        return false;
    }

    if (size < OWNER_LENGTH + MIN_TOKEN_LENGTH) {
        errorMsg = "bad-txns-owner-token-length";
        return false;
    }

    if (ownerName != std::string(tokenName + OWNER_TAG)) {
        errorMsg = "bad-txns-owner-name-mismatch";
        return false;
    }

    return true;
}

bool OwnerFromTransaction(const CTransaction& tx, std::string& ownerName, std::string& strAddress)
{
    // Check to see if the transaction is an new token issue tx
    if (!tx.IsNewToken())
        return false;

    // Get the scriptPubKey from the last tx in vout
    CScript scriptPubKey = tx.vout[tx.vout.size() - 2].scriptPubKey;

    return OwnerTokenFromScript(scriptPubKey, ownerName, strAddress);
}

bool TransferTokenFromScript(const CScript& scriptPubKey, CTokenTransfer& tokenTransfer, std::string& strAddress)
{
    int nStartingIndex = 0;
    if (!IsScriptTransferToken(scriptPubKey, nStartingIndex))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchTransferToken;
    vchTransferToken.insert(vchTransferToken.end(), scriptPubKey.begin() + 31, scriptPubKey.end());
    CDataStream ssToken(vchTransferToken, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssToken >> tokenTransfer;
    } catch(std::exception& e) {
        std::cout << "Failed to get the transfer token from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool TokenFromScript(const CScript& scriptPubKey, CNewToken& tokenNew, std::string& strAddress)
{
    int nStartingIndex = 0;
    if (!IsScriptNewToken(scriptPubKey, nStartingIndex))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchNewToken;
    vchNewToken.insert(vchNewToken.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssToken(vchNewToken, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssToken >> tokenNew;
    } catch(std::exception& e) {
        std::cout << "Failed to get the token from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool OwnerTokenFromScript(const CScript& scriptPubKey, std::string& tokenName, std::string& strAddress)
{
    int nStartingIndex = 0;
    if (!IsScriptOwnerToken(scriptPubKey, nStartingIndex))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchOwnerToken;
    vchOwnerToken.insert(vchOwnerToken.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssOwner(vchOwnerToken, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssOwner >> tokenName;
    } catch(std::exception& e) {
        std::cout << "Failed to get the owner token from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

bool ReissueTokenFromScript(const CScript& scriptPubKey, CReissueToken& reissue, std::string& strAddress)
{
    int nStartingIndex = 0;
    if (!IsScriptReissueToken(scriptPubKey, nStartingIndex))
        return false;

    CTxDestination destination;
    ExtractDestination(scriptPubKey, destination);

    strAddress = EncodeDestination(destination);

    std::vector<unsigned char> vchReissueToken;
    vchReissueToken.insert(vchReissueToken.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssReissue(vchReissueToken, SER_NETWORK, PROTOCOL_VERSION);

    try {
        ssReissue >> reissue;
    } catch(std::exception& e) {
        std::cout << "Failed to get the reissue token from the stream: " << e.what() << std::endl;
        return false;
    }

    return true;
}

//! Call VerifyNewToken if this function returns true
bool CTransaction::IsNewToken() const
{
    // Check for the tokens data CTxOut. This will always be the last output in the transaction
    if (!CheckIssueDataTx(vout[vout.size() - 1]))
        return false;

    // Check to make sure the owner token is created
    if (!CheckOwnerDataTx(vout[vout.size() - 2]))
        return false;

    // Don't overlap with IsNewUniqueToken()
    if (IsScriptNewUniqueToken(vout[vout.size() - 1].scriptPubKey))
        return false;

    return true;
}

//! To be called on CTransactions where IsNewToken returns true
bool CTransaction::VerifyNewToken(std::string& strError) const
{
    // Issuing an Token must contain at least 3 CTxOut( Alphacon Burn Tx, Any Number of other Outputs ..., Owner Token Tx, New Token Tx)
    if (vout.size() < 3) {
        strError  = "bad-txns-issue-vout-size-to-small";
        return false;
    }

    // Check for the tokens data CTxOut. This will always be the last output in the transaction
    if (!CheckIssueDataTx(vout[vout.size() - 1])) {
        strError  = "bad-txns-issue-data-not-found";
        return false;
    }

    // Check to make sure the owner token is created
    if (!CheckOwnerDataTx(vout[vout.size() - 2])) {
        strError  = "bad-txns-issue-owner-data-not-found";
        return false;
    }

    // Get the token type
    CNewToken token;
    std::string address;
    if (!TokenFromScript(vout[vout.size() - 1].scriptPubKey, token, address)) {
        strError = "bad-txns-issue-serialzation-failed";
        return error("%s : Failed to get new token from transaction: %s", __func__, this->GetHash().GetHex());
    }

    TokenType tokenType;
    IsTokenNameValid(token.strName, tokenType);

    std::string strOwnerName;
    if (!OwnerTokenFromScript(vout[vout.size() - 2].scriptPubKey, strOwnerName, address)) {
        strError = "bad-txns-issue-owner-serialzation-failed";
        return false;
    }

    if (strOwnerName != token.strName + OWNER_TAG) {
        strError = "bad-txns-issue-owner-name-doesn't-match";
        return false;
    }

    // Check for the Burn CTxOut in one of the vouts ( This is needed because the change CTxOut is places in a random position in the CWalletTx
    bool fFoundIssueBurnTx = false;
    for (auto out : vout) {
        if (CheckIssueBurnTx(out, tokenType)) {
            fFoundIssueBurnTx = true;
            break;
        }
    }

    if (!fFoundIssueBurnTx) {
        strError = "bad-txns-issue-burn-not-found";
        return false;
    }

    // Loop through all of the vouts and make sure only the expected token creations are taking place
    int nTransfers = 0;
    int nOwners = 0;
    int nIssues = 0;
    int nReissues = 0;
    GetTxOutTokenTypes(vout, nIssues, nReissues, nTransfers, nOwners);

    if (nOwners != 1 || nIssues != 1 || nReissues > 0) {
        strError = "bad-txns-failed-issue-token-formatting-check";
        return false;
    }

    return true;
}

//! Make sure to call VerifyNewUniqueToken if this call returns true
bool CTransaction::IsNewUniqueToken() const
{
    // Check trailing outpoint for issue data with unique token name
    if (!CheckIssueDataTx(vout[vout.size() - 1]))
        return false;

    if (!IsScriptNewUniqueToken(vout[vout.size() - 1].scriptPubKey))
        return false;

    return true;
}

//! Call this function after IsNewUniqueToken
bool CTransaction::VerifyNewUniqueToken(std::string& strError) const
{
    // Must contain at least 3 outpoints (ALP burn, owner change and one or more new unique tokens that share a root (should be in trailing position))
    if (vout.size() < 3) {
        strError  = "bad-txns-unique-vout-size-to-small";
        return false;
    }

    // check for (and count) new unique token outpoints.  make sure they share a root.
    std::set<std::string> setUniqueTokens;
    std::string tokenRoot = "";
    int tokenOutpointCount = 0;

    for (auto out : vout) {
        if (IsScriptNewUniqueToken(out.scriptPubKey)) {
            CNewToken token;
            std::string address;
            if (!TokenFromScript(out.scriptPubKey, token, address)) {
                strError = "bad-txns-issue-unique-token-from-script";
                return false;
            }
            std::string root = GetParentName(token.strName);
            if (tokenRoot.compare("") == 0)
                tokenRoot = root;
            if (tokenRoot.compare(root) != 0) {
                strError = "bad-txns-issue-unique-token-compare-failed";
                return false;
            }

            // Check for duplicate unique tokens in the same transaction
            if (setUniqueTokens.count(token.strName)) {
                strError = "bad-txns-issue-unique-duplicate-name-in-same-tx";
                return false;
            }

            setUniqueTokens.insert(token.strName);
            tokenOutpointCount += 1;
        }
    }

    if (tokenOutpointCount == 0) {
        strError = "bad-txns-issue-unique-token-bad-outpoint-count";
        return false;
    }

    // check for burn outpoint (must account for each new token)
    bool fBurnOutpointFound = false;
    for (auto out : vout) {
        if (CheckIssueBurnTx(out, TokenType::UNIQUE, tokenOutpointCount)) {
            fBurnOutpointFound = true;
            break;
        }
    }

    if (!fBurnOutpointFound) {
        strError = "bad-txns-issue-unique-token-burn-outpoints-not-found";
        return false;
    }

    // check for owner change outpoint that matches root
    bool fOwnerOutFound = false;
    for (auto out : vout) {
        CTokenTransfer transfer;
        std::string transferAddress;
        if (TransferTokenFromScript(out.scriptPubKey, transfer, transferAddress)) {
            if (tokenRoot + OWNER_TAG == transfer.strName) {
                fOwnerOutFound = true;
                break;
            }
        }
    }

    if (!fOwnerOutFound) {
        strError = "bad-txns-issue-unique-token-bad-owner-token";
        return false;
    }

    // Loop through all of the vouts and make sure only the expected token creations are taking place
    int nTransfers = 0;
    int nOwners = 0;
    int nIssues = 0;
    int nReissues = 0;
    GetTxOutTokenTypes(vout, nIssues, nReissues, nTransfers, nOwners);

    if (nOwners > 0 || nReissues > 0 || nIssues != tokenOutpointCount) {
        strError = "bad-txns-failed-unique-token-formatting-check";
        return false;
    }

    return true;
}

bool CTransaction::IsReissueToken() const
{
    // Check for the reissue token data CTxOut. This will always be the last output in the transaction
    if (!CheckReissueDataTx(vout[vout.size() - 1]))
        return false;

    return true;
}

//! To be called on CTransactions where IsReissueToken returns true
bool CTransaction::VerifyReissueToken(std::string& strError) const
{
    // Reissuing an Token must contain at least 3 CTxOut ( Alphacon Burn Tx, Any Number of other Outputs ..., Reissue Token Tx, Owner Token Change Tx)
    if (vout.size() < 3) {
        strError  = "bad-txns-vout-size-to-small";
        return false;
    }

    // Check for the reissue token data CTxOut. This will always be the last output in the transaction
    if (!CheckReissueDataTx(vout[vout.size() - 1])) {
        strError  = "bad-txns-reissue-data-not-found";
        return false;
    }

    CReissueToken reissue;
    std::string address;
    if (!ReissueTokenFromScript(vout[vout.size() - 1].scriptPubKey, reissue, address)) {
        strError  = "bad-txns-reissue-serialization-failed";
        return false;
    }

    // Check that there is an token transfer, this will be the owner token change
    bool fOwnerOutFound = false;
    for (auto out : vout) {
        CTokenTransfer transfer;
        std::string transferAddress;
        if (TransferTokenFromScript(out.scriptPubKey, transfer, transferAddress)) {
            if (reissue.strName + OWNER_TAG == transfer.strName) {
                fOwnerOutFound = true;
                break;
            }
        }
    }

    if (!fOwnerOutFound) {
        strError  = "bad-txns-reissue-owner-outpoint-not-found";
        return false;
    }

    // Check for the Burn CTxOut in one of the vouts ( This is needed because the change CTxOut is placed in a random position in the CWalletTx
    bool fFoundReissueBurnTx = false;
    for (auto out : vout) {
        if (CheckReissueBurnTx(out)) {
            fFoundReissueBurnTx = true;
            break;
        }
    }

    if (!fFoundReissueBurnTx) {
        strError = "bad-txns-reissue-burn-outpoint-not-found";
        return false;
    }

    // Loop through all of the vouts and make sure only the expected token creations are taking place
    int nTransfers = 0;
    int nOwners = 0;
    int nIssues = 0;
    int nReissues = 0;
    GetTxOutTokenTypes(vout, nIssues, nReissues, nTransfers, nOwners);

    if (nOwners > 0 || nReissues != 1 || nIssues > 0) {
        strError = "bad-txns-failed-reissue-token-formatting-check";
        return false;
    }

    return true;
}

CTokenTransfer::CTokenTransfer(const std::string& strTokenName, const CAmount& nAmount, const uint32_t& nTokenLockTime)
{
    this->strName = strTokenName;
    this->nAmount = nAmount;
    this->nTokenLockTime = nTokenLockTime;
}

bool CTokenTransfer::IsValid(std::string& strError) const
{
    strError = "";

    if (!IsTokenNameValid(std::string(strName)))
        strError = "Invalid parameter: token_name must only consist of valid characters and have a size between 3 and 30 characters. See help for more details.";

    if (nAmount <= 0)
        strError  = "Invalid parameter: token amount can't be equal to or less than zero.";

    return strError == "";
}

void CTokenTransfer::ConstructTransaction(CScript& script) const
{
    CDataStream ssTransfer(SER_NETWORK, PROTOCOL_VERSION);
    ssTransfer << *this;

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(ALP_A); // a
    vchMessage.push_back(ALP_L); // l
    vchMessage.push_back(ALP_P); // p
    vchMessage.push_back(ALP_T); // t

    vchMessage.insert(vchMessage.end(), ssTransfer.begin(), ssTransfer.end());
    script << OP_ALP_TOKEN << ToByteVector(vchMessage) << OP_DROP;
}

CReissueToken::CReissueToken(const std::string &strTokenName, const CAmount &nAmount, const int &nUnits, const int &nReissuable,
                             const std::string &strIPFSHash)
{

    this->strName = strTokenName;
    this->strIPFSHash = strIPFSHash;
    this->nReissuable = int8_t(nReissuable);
    this->nAmount = nAmount;
    this->nUnits = nUnits;
}

bool CReissueToken::IsValid(std::string &strError, CTokensCache& tokenCache, bool fForceCheckPrimaryTokenExists) const {
    strError = "";

    if (fForceCheckPrimaryTokenExists) {
        CNewToken token;
        if (!tokenCache.GetTokenMetaDataIfExists(this->strName, token)) {
            strError = _("Unable to reissue token: token_name '") + strName + _("' doesn't exist in the database");
            return false;
        }

        if (!token.nReissuable) {
            // Check to make sure the token can be reissued
            strError = _("Unable to reissue token: reissuable is set to false");
            return false;
        }

        if (token.nAmount + this->nAmount > MAX_MONEY_TOKENS) {
            strError = _("Unable to reissue token: token_name '") + strName +
                       _("' the amount trying to reissue is to large");
            return false;
        }

        if (!CheckAmountWithUnits(nAmount, token.units)) {
            strError = _("Unable to reissue token: amount must be divisible by the smaller unit assigned to the token");
            return false;
        }

        if (nUnits < token.units && nUnits != -1) {
            strError = _("Unable to reissue token: unit must be larger than current unit selection");
            return false;
        }
    }

    if (strIPFSHash != "") {
        if (!CheckEncodedIPFS(EncodeIPFS(strIPFSHash), strError))
            return false;
    }

    if (nAmount < 0) {
        strError = _("Unable to reissue token: amount must be 0 or larger");
        return false;
    }

    if (nUnits > MAX_UNIT || nUnits < -1) {
        strError = _("Unable to reissue token: unit must be between 8 and -1");
        return false;
    }

    return true;
}

void CReissueToken::ConstructTransaction(CScript& script) const
{
    CDataStream ssReissue(SER_NETWORK, PROTOCOL_VERSION);
    ssReissue << *this;

    std::vector<unsigned char> vchMessage;
    vchMessage.push_back(ALP_A); // a
    vchMessage.push_back(ALP_L); // l
    vchMessage.push_back(ALP_P); // p
    vchMessage.push_back(ALP_A); // a

    vchMessage.insert(vchMessage.end(), ssReissue.begin(), ssReissue.end());
    script << OP_ALP_TOKEN << ToByteVector(vchMessage) << OP_DROP;
}

bool CReissueToken::IsNull() const
{
    return strName == "" || nAmount < 0;
}

bool CTokensCache::AddTransferToken(const CTokenTransfer& transferToken, const std::string& address, const COutPoint& out, const CTxOut& txOut)
{
    AddToTokenBalance(transferToken.strName, address, transferToken.nAmount);

    // Add to cache so we can save to database
    CTokenCacheNewTransfer newTransfer(CTokenTransfer(transferToken.strName, transferToken.nAmount, transferToken.nTokenLockTime), address, out);

    if (setNewTransferTokensToRemove.count(newTransfer))
        setNewTransferTokensToRemove.erase(newTransfer);

    setNewTransferTokensToAdd.insert(newTransfer);

    return true;
}

void CTokensCache::AddToTokenBalance(const std::string& strName, const std::string& address, const CAmount& nAmount)
{
    if (fTokenIndex) {
        auto pair = std::make_pair(strName, address);
        // Add to map address -> amount map

        // Get the best amount
        if (!GetBestTokenAddressAmount(*this, strName, address))
            mapTokensAddressAmount.insert(make_pair(pair, 0));

        // Add the new amount to the balance
        if (IsTokenNameAnOwner(strName))
            mapTokensAddressAmount.at(pair) = OWNER_TOKEN_AMOUNT;
        else
            mapTokensAddressAmount.at(pair) += nAmount;
    }
}

bool CTokensCache::TrySpendCoin(const COutPoint& out, const CTxOut& txOut)
{
    // Placeholder strings that will get set if you successfully get the transfer or token from the script
    std::string address = "";
    std::string tokenName = "";
    CAmount nAmount = -1;

    // Get the token tx data
    int nType = -1;
    bool fIsOwner = false;
    if (txOut.scriptPubKey.IsTokenScript(nType, fIsOwner)) {

        // Get the New Token or Transfer Token from the scriptPubKey
        if (nType == TX_NEW_TOKEN && !fIsOwner) {
            CNewToken token;
            if (TokenFromScript(txOut.scriptPubKey, token, address)) {
                tokenName = token.strName;
                nAmount = token.nAmount;
            }
        } else if (nType == TX_TRANSFER_TOKEN) {
            CTokenTransfer transfer;
            if (TransferTokenFromScript(txOut.scriptPubKey, transfer, address)) {
                tokenName = transfer.strName;
                nAmount = transfer.nAmount;
            }
        } else if (nType == TX_NEW_TOKEN && fIsOwner) {
            if (!OwnerTokenFromScript(txOut.scriptPubKey, tokenName, address))
                return error("%s : ERROR Failed to get owner token from the OutPoint: %s", __func__,
                             out.ToString());
            nAmount = OWNER_TOKEN_AMOUNT;
        } else if (nType == TX_REISSUE_TOKEN) {
            CReissueToken reissue;
            if (ReissueTokenFromScript(txOut.scriptPubKey, reissue, address)) {
                tokenName = reissue.strName;
                nAmount = reissue.nAmount;
            }
        }
    } else {
        // If it isn't an token tx return true, we only fail if an error occurs
        return true;
    }

    // If we got the address and the tokenName, proceed to remove it from the database, and in memory objects
    if (address != "" && tokenName != "" && nAmount > 0) {
        if (fTokenIndex) {
            CTokenCacheSpendToken spend(tokenName, address, nAmount);
            if (GetBestTokenAddressAmount(*this, tokenName, address)) {
                auto pair = make_pair(tokenName, address);
                if (mapTokensAddressAmount.count(pair))
                    mapTokensAddressAmount.at(pair) -= nAmount;

                if (mapTokensAddressAmount.at(pair) < 0)
                    mapTokensAddressAmount.at(pair) = 0;

                // Update the cache so we can save to database
                vSpentTokens.push_back(spend);
            }
        }
    } else {
        return error("%s : ERROR Failed to get token from the OutPoint: %s", __func__, out.ToString());
    }

    return true;
}

bool CTokensCache::ContainsToken(const CNewToken& token)
{
    return CheckIfTokenExists(token.strName);
}

bool CTokensCache::ContainsToken(const std::string& tokenName)
{
    return CheckIfTokenExists(tokenName);
}

bool CTokensCache::UndoTokenCoin(const Coin& coin, const COutPoint& out)
{
    std::string strAddress = "";
    std::string tokenName = "";
    CAmount nAmount = 0;

    // Get the token tx from the script
    int nType = -1;
    bool fIsOwner = false;
    if(coin.out.scriptPubKey.IsTokenScript(nType, fIsOwner)) {

        if (nType == TX_NEW_TOKEN && !fIsOwner) {
            CNewToken token;
            if (!TokenFromScript(coin.out.scriptPubKey, token, strAddress)) {
                return error("%s : Failed to get token from script while trying to undo token spend. OutPoint : %s",
                             __func__,
                             out.ToString());
            }
            tokenName = token.strName;

            nAmount = token.nAmount;
        } else if (nType == TX_TRANSFER_TOKEN) {
            CTokenTransfer transfer;
            if (!TransferTokenFromScript(coin.out.scriptPubKey, transfer, strAddress))
                return error(
                        "%s : Failed to get transfer token from script while trying to undo token spend. OutPoint : %s",
                        __func__,
                        out.ToString());

            tokenName = transfer.strName;
            nAmount = transfer.nAmount;
        } else if (nType == TX_NEW_TOKEN && fIsOwner) {
            std::string ownerName;
            if (!OwnerTokenFromScript(coin.out.scriptPubKey, ownerName, strAddress))
                return error(
                        "%s : Failed to get owner token from script while trying to undo token spend. OutPoint : %s",
                        __func__, out.ToString());
            tokenName = ownerName;
            nAmount = OWNER_TOKEN_AMOUNT;
        } else if (nType == TX_REISSUE_TOKEN) {
            CReissueToken reissue;
            if (!ReissueTokenFromScript(coin.out.scriptPubKey, reissue, strAddress))
                return error(
                        "%s : Failed to get reissue token from script while trying to undo token spend. OutPoint : %s",
                        __func__, out.ToString());
            tokenName = reissue.strName;
            nAmount = reissue.nAmount;
        }
    }

    if (tokenName == "" || strAddress == "" || nAmount == 0)
        return error("%s : TokenName, Address or nAmount is invalid., Token Name: %s, Address: %s, Amount: %d", __func__, tokenName, strAddress, nAmount);

    if (!AddBackSpentToken(coin, tokenName, strAddress, nAmount, out))
        return error("%s : Failed to add back the spent token. OutPoint : %s", __func__, out.ToString());

    return true;
}

//! Changes Memory Only
bool CTokensCache::AddBackSpentToken(const Coin& coin, const std::string& tokenName, const std::string& address, const CAmount& nAmount, const COutPoint& out)
{
    if (fTokenIndex) {
        // Update the tokens address balance
        auto pair = std::make_pair(tokenName, address);

        // Get the map address amount from database if the map doesn't have it already
        if (!GetBestTokenAddressAmount(*this, tokenName, address))
            mapTokensAddressAmount.insert(std::make_pair(pair, 0));

        mapTokensAddressAmount.at(pair) += nAmount;
    }

    // Add the undoAmount to the vector so we know what changes are dirty and what needs to be saved to database
    CTokenCacheUndoTokenAmount undoAmount(tokenName, address, nAmount);
    vUndoTokenAmount.push_back(undoAmount);

    return true;
}

//! Changes Memory Only
bool CTokensCache::UndoTransfer(const CTokenTransfer& transfer, const std::string& address, const COutPoint& outToRemove)
{
    if (fTokenIndex) {
        // Make sure we are in a valid state to undo the transfer of the token
        if (!GetBestTokenAddressAmount(*this, transfer.strName, address))
            return error("%s : Failed to get the tokens address balance from the database. Token : %s Address : %s",
                         __func__, transfer.strName, address);

        auto pair = std::make_pair(transfer.strName, address);
        if (!mapTokensAddressAmount.count(pair))
            return error(
                    "%s : Tried undoing a transfer and the map of address amount didn't have the token address pair. Token : %s Address : %s",
                    __func__, transfer.strName, address);

        if (mapTokensAddressAmount.at(pair) < transfer.nAmount)
            return error(
                    "%s : Tried undoing a transfer and the map of address amount had less than the amount we are trying to undo. Token : %s Address : %s",
                    __func__, transfer.strName, address);

        // Change the in memory balance of the token at the address
        mapTokensAddressAmount[pair] -= transfer.nAmount;
    }

    return true;
}

//! Changes Memory Only
bool CTokensCache::RemoveNewToken(const CNewToken& token, const std::string address)
{
    if (!CheckIfTokenExists(token.strName))
        return error("%s : Tried removing an token that didn't exist. Token Name : %s", __func__, token.strName);

    CTokenCacheNewToken newToken(token, address, 0 , uint256());

    if (setNewTokensToAdd.count(newToken))
        setNewTokensToAdd.erase(newToken);

    setNewTokensToRemove.insert(newToken);

    if (fTokenIndex)
        mapTokensAddressAmount[std::make_pair(token.strName, address)] = 0;

    return true;
}

//! Changes Memory Only
bool CTokensCache::AddNewToken(const CNewToken& token, const std::string address, const int& nHeight, const uint256& blockHash)
{
    if(CheckIfTokenExists(token.strName))
        return error("%s: Tried adding new token, but it already existed in the set of tokens: %s", __func__, token.strName);

    CTokenCacheNewToken newToken(token, address, nHeight, blockHash);

    if (setNewTokensToRemove.count(newToken))
        setNewTokensToRemove.erase(newToken);

    setNewTokensToAdd.insert(newToken);

    if (fTokenIndex) {
        // Insert the token into the assests address amount map
        mapTokensAddressAmount[std::make_pair(token.strName, address)] = token.nAmount;
    }

    return true;
}

//! Changes Memory Only
bool CTokensCache::AddReissueToken(const CReissueToken& reissue, const std::string address, const COutPoint& out)
{
    auto pair = std::make_pair(reissue.strName, address);

    CNewToken token;
    int tokenHeight;
    uint256 tokenBlockHash;
    if (!GetTokenMetaDataIfExists(reissue.strName, token, tokenHeight, tokenBlockHash))
        return error("%s: Failed to get the original token that is getting reissued. Token Name : %s",
                     __func__, reissue.strName);

    // Insert the reissue information into the reissue map
    if (!mapReissuedTokenData.count(reissue.strName)) {
        token.nAmount += reissue.nAmount;
        token.nReissuable = reissue.nReissuable;
        if (reissue.nUnits != -1)
            token.units = reissue.nUnits;

        if (reissue.strIPFSHash != "") {
            return error("%s: This function is disabled. Token Name : %s",
                     __func__, reissue.strName);
        }
        mapReissuedTokenData.insert(make_pair(reissue.strName, token));
    } else {
        mapReissuedTokenData.at(reissue.strName).nAmount += reissue.nAmount;
        mapReissuedTokenData.at(reissue.strName).nReissuable = reissue.nReissuable;
        if (reissue.nUnits != -1) {
            mapReissuedTokenData.at(reissue.strName).units = reissue.nUnits;
        }
        if (reissue.strIPFSHash != "") {
            return error("%s: This function is disabled. Token Name : %s",
                     __func__, reissue.strName);
        }
    }

    CTokenCacheReissueToken reissueToken(reissue, address, out, tokenHeight, tokenBlockHash);

    if (setNewReissueToRemove.count(reissueToken))
        setNewReissueToRemove.erase(reissueToken);

    setNewReissueToAdd.insert(reissueToken);

    if (fTokenIndex) {
        // Add the reissued amount to the address amount map
        if (!GetBestTokenAddressAmount(*this, reissue.strName, address))
            mapTokensAddressAmount.insert(make_pair(pair, 0));

        // Add the reissued amount to the amount in the map
        mapTokensAddressAmount[pair] += reissue.nAmount;
    }

    return true;
}

//! Changes Memory Only
bool CTokensCache::RemoveReissueToken(const CReissueToken& reissue, const std::string address, const COutPoint& out, const std::vector<std::pair<std::string, CBlockTokenUndo> >& vUndoIPFS)
{
    auto pair = std::make_pair(reissue.strName, address);

    CNewToken tokenData;
    int height;
    uint256 blockHash;
    if (!GetTokenMetaDataIfExists(reissue.strName, tokenData, height, blockHash))
        return error("%s: Tried undoing reissue of an token, but that token didn't exist: %s", __func__, reissue.strName);

    // Change the token data by undoing what was reissued
    tokenData.nAmount -= reissue.nAmount;
    tokenData.nReissuable = 1;

    // Find the ipfs hash in the undoblock data and restore the ipfs hash to its previous hash
    for (auto undoItem : vUndoIPFS) {
        if (undoItem.first == reissue.strName) {
            if (undoItem.second.fChangedIPFS)
                tokenData.strIPFSHash = undoItem.second.strIPFS;
            if(undoItem.second.fChangedUnits)
                tokenData.units = undoItem.second.nUnits;
            if (tokenData.strIPFSHash == "")
                tokenData.nHasIPFS = 0;
            break;
        }
    }

    mapReissuedTokenData[tokenData.strName] = tokenData;

    CTokenCacheReissueToken reissueToken(reissue, address, out, height, blockHash);

    if (setNewReissueToAdd.count(reissueToken))
        setNewReissueToAdd.erase(reissueToken);

    setNewReissueToRemove.insert(reissueToken);

    if (fTokenIndex) {
        // Get the best amount form the database or dirty cache
        if (!GetBestTokenAddressAmount(*this, reissue.strName, address))
            return error("%s : Trying to undo reissue of an token but the tokens amount isn't in the database",
                         __func__);

        mapTokensAddressAmount[pair] -= reissue.nAmount;

        if (mapTokensAddressAmount[pair] < 0)
            return error("%s : Tried undoing reissue of an token, but the tokens amount went negative: %s", __func__,
                         reissue.strName);
    }

    return true;
}

//! Changes Memory Only
bool CTokensCache::AddOwnerToken(const std::string& tokensName, const std::string address)
{
    // Update the cache
    CTokenCacheNewOwner newOwner(tokensName, address);

    if (setNewOwnerTokensToRemove.count(newOwner))
        setNewOwnerTokensToRemove.erase(newOwner);

    setNewOwnerTokensToAdd.insert(newOwner);

    if (fTokenIndex) {
        // Insert the token into the assests address amount map
        mapTokensAddressAmount[std::make_pair(tokensName, address)] = OWNER_TOKEN_AMOUNT;
    }

    return true;
}

//! Changes Memory Only
bool CTokensCache::RemoveOwnerToken(const std::string& tokensName, const std::string address)
{
    // Update the cache
    CTokenCacheNewOwner newOwner(tokensName, address);
    if (setNewOwnerTokensToAdd.count(newOwner))
        setNewOwnerTokensToAdd.erase(newOwner);

    setNewOwnerTokensToRemove.insert(newOwner);

    if (fTokenIndex) {
        auto pair = std::make_pair(tokensName, address);
        mapTokensAddressAmount[pair] = 0;
    }

    return true;
}

//! Changes Memory Only
bool CTokensCache::RemoveTransfer(const CTokenTransfer &transfer, const std::string &address, const COutPoint &out)
{
    if (!UndoTransfer(transfer, address, out))
        return error("%s : Failed to undo the transfer", __func__);

    CTokenCacheNewTransfer newTransfer(transfer, address, out);
    if (setNewTransferTokensToAdd.count(newTransfer))
        setNewTransferTokensToAdd.erase(newTransfer);

    setNewTransferTokensToRemove.insert(newTransfer);

    return true;
}

bool CTokensCache::DumpCacheToDatabase()
{
    try {
        bool dirty = false;
        std::string message;

        // Remove new tokens from the database
        for (auto newToken : setNewTokensToRemove) {
            ptokensCache->Erase(newToken.token.strName);
            if (!ptokensdb->EraseTokenData(newToken.token.strName)) {
                dirty = true;
                message = "_Failed Erasing New Token Data from database";
            }

            if (dirty) {
                return error("%s : %s", __func__, message);
            }

            if (fTokenIndex) {
                if (!ptokensdb->EraseTokenAddressQuantity(newToken.token.strName, newToken.address)) {
                    dirty = true;
                    message = "_Failed Erasing Address Balance from database";
                }

                if (!ptokensdb->EraseAddressTokenQuantity(newToken.address, newToken.token.strName)) {
                    dirty = true;
                    message = "_Failed Erasing New Token Address Balance from AddressToken database";
                }
            }

            if (dirty) {
                return error("%s : %s", __func__, message);
            }
        }

        // Add the new tokens to the database
        for (auto newToken : setNewTokensToAdd) {
            ptokensCache->Put(newToken.token.strName, CDatabasedTokenData(newToken.token, newToken.blockHeight, newToken.blockHash));
            if (!ptokensdb->WriteTokenData(newToken.token, newToken.blockHeight, newToken.blockHash)) {
                dirty = true;
                message = "_Failed Writing New Token Data to database";
            }

            if (dirty) {
                return error("%s : %s", __func__, message);
            }

            if (fTokenIndex) {
                if (!ptokensdb->WriteTokenAddressQuantity(newToken.token.strName, newToken.address,
                                                          newToken.token.nAmount)) {
                    dirty = true;
                    message = "_Failed Writing Address Balance to database";
                }

                if (!ptokensdb->WriteAddressTokenQuantity(newToken.address, newToken.token.strName,
                                                          newToken.token.nAmount)) {
                    dirty = true;
                    message = "_Failed Writing Address Balance to database";
                }
            }

            if (dirty) {
                return error("%s : %s", __func__, message);
            }
        }

        if (fTokenIndex) {
            // Remove the new owners from database
            for (auto ownerToken : setNewOwnerTokensToRemove) {
                if (!ptokensdb->EraseTokenAddressQuantity(ownerToken.tokenName, ownerToken.address)) {
                    dirty = true;
                    message = "_Failed Erasing Owner Address Balance from database";
                }

                if (!ptokensdb->EraseAddressTokenQuantity(ownerToken.address, ownerToken.tokenName)) {
                    dirty = true;
                    message = "_Failed Erasing New Owner Address Balance from AddressToken database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }
            }

            // Add the new owners to database
            for (auto ownerToken : setNewOwnerTokensToAdd) {
                auto pair = std::make_pair(ownerToken.tokenName, ownerToken.address);
                if (mapTokensAddressAmount.count(pair) && mapTokensAddressAmount.at(pair) > 0) {
                    if (!ptokensdb->WriteTokenAddressQuantity(ownerToken.tokenName, ownerToken.address,
                                                              mapTokensAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing Owner Address Balance to database";
                    }

                    if (!ptokensdb->WriteAddressTokenQuantity(ownerToken.address, ownerToken.tokenName,
                                                              mapTokensAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing Address Balance to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }
                }
            }

            // Undo the transfering by updating the balances in the database

            for (auto undoTransfer : setNewTransferTokensToRemove) {
                auto pair = std::make_pair(undoTransfer.transfer.strName, undoTransfer.address);
                if (mapTokensAddressAmount.count(pair)) {
                    if (mapTokensAddressAmount.at(pair) == 0) {
                        if (!ptokensdb->EraseTokenAddressQuantity(undoTransfer.transfer.strName,
                                                                  undoTransfer.address)) {
                            dirty = true;
                            message = "_Failed Erasing Address Quantity from database";
                        }

                        if (!ptokensdb->EraseAddressTokenQuantity(undoTransfer.address,
                                                                  undoTransfer.transfer.strName)) {
                            dirty = true;
                            message = "_Failed Erasing UndoTransfer Address Balance from AddressToken database";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    } else {
                        if (!ptokensdb->WriteTokenAddressQuantity(undoTransfer.transfer.strName,
                                                                  undoTransfer.address,
                                                                  mapTokensAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Writing updated Address Quantity to database when undoing transfers";
                        }

                        if (!ptokensdb->WriteAddressTokenQuantity(undoTransfer.address,
                                                                  undoTransfer.transfer.strName,
                                                                  mapTokensAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Writing Address Balance to database";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    }
                }
            }


            // Save the new transfers by updating the quantity in the database
            for (auto newTransfer : setNewTransferTokensToAdd) {
                auto pair = std::make_pair(newTransfer.transfer.strName, newTransfer.address);
                // During init and reindex it disconnects and verifies blocks, can create a state where vNewTransfer will contain transfers that have already been spent. So if they aren't in the map, we can skip them.
                if (mapTokensAddressAmount.count(pair)) {
                    if (!ptokensdb->WriteTokenAddressQuantity(newTransfer.transfer.strName, newTransfer.address,
                                                              mapTokensAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing new address quantity to database";
                    }

                    if (!ptokensdb->WriteAddressTokenQuantity(newTransfer.address, newTransfer.transfer.strName,
                                                              mapTokensAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing Address Balance to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }
                }
            }
        }

        for (auto newReissue : setNewReissueToAdd) {
            auto reissue_name = newReissue.reissue.strName;
            auto pair = make_pair(reissue_name, newReissue.address);
            if (mapReissuedTokenData.count(reissue_name)) {
                if(!ptokensdb->WriteTokenData(mapReissuedTokenData.at(reissue_name), newReissue.blockHeight, newReissue.blockHash)) {
                    dirty = true;
                    message = "_Failed Writing reissue token data to database";
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }

                ptokensCache->Erase(reissue_name);

                if (fTokenIndex) {
                    if (mapTokensAddressAmount.count(pair) && mapTokensAddressAmount.at(pair) > 0) {
                        if (!ptokensdb->WriteTokenAddressQuantity(pair.first, pair.second,
                                                                  mapTokensAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Writing reissue token quantity to the address quantity database";
                        }

                        if (!ptokensdb->WriteAddressTokenQuantity(pair.second, pair.first,
                                                                  mapTokensAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Writing Address Balance to database";
                        }

                        if (dirty) {
                            return error("%s, %s", __func__, message);
                        }
                    }
                }
            }
        }

        for (auto undoReissue : setNewReissueToRemove) {
            // In the case the the issue and reissue are both being removed
            // we can skip this call because the removal of the issue should remove all data pertaining the to token
            // Fixes the issue where the reissue data will write over the removed token meta data that was removed above
            CNewToken token(undoReissue.reissue.strName, 0);
            CTokenCacheNewToken testNewTokenCache(token, "", 0 , uint256());
            if (setNewTokensToRemove.count(testNewTokenCache)) {
                continue;
            }

            auto reissue_name = undoReissue.reissue.strName;
            if (mapReissuedTokenData.count(reissue_name)) {
                if(!ptokensdb->WriteTokenData(mapReissuedTokenData.at(reissue_name), undoReissue.blockHeight, undoReissue.blockHash)) {
                    dirty = true;
                    message = "_Failed Writing undo reissue token data to database";
                }

                if (fTokenIndex) {
                    auto pair = make_pair(undoReissue.reissue.strName, undoReissue.address);
                    if (mapTokensAddressAmount.count(pair)) {
                        if (mapTokensAddressAmount.at(pair) == 0) {
                            if (!ptokensdb->EraseTokenAddressQuantity(reissue_name, undoReissue.address)) {
                                dirty = true;
                                message = "_Failed Erasing Address Balance from database";
                            }

                            if (!ptokensdb->EraseAddressTokenQuantity(undoReissue.address, reissue_name)) {
                                dirty = true;
                                message = "_Failed Erasing UndoReissue Balance from AddressToken database";
                            }
                        } else {
                            if (!ptokensdb->WriteTokenAddressQuantity(reissue_name, undoReissue.address,
                                                                      mapTokensAddressAmount.at(pair))) {
                                dirty = true;
                                message = "_Failed Writing the undo of reissue of token from database";
                            }

                            if (!ptokensdb->WriteAddressTokenQuantity(undoReissue.address, reissue_name,
                                                                      mapTokensAddressAmount.at(pair))) {
                                dirty = true;
                                message = "_Failed Writing Address Balance to database";
                            }
                        }
                    }
                }

                if (dirty) {
                    return error("%s : %s", __func__, message);
                }

                ptokensCache->Erase(reissue_name);
            }
        }

        if (fTokenIndex) {
            // Undo the token spends by updating there balance in the database
            for (auto undoSpend : vUndoTokenAmount) {
                auto pair = std::make_pair(undoSpend.tokenName, undoSpend.address);
                if (mapTokensAddressAmount.count(pair)) {
                    if (!ptokensdb->WriteTokenAddressQuantity(undoSpend.tokenName, undoSpend.address,
                                                              mapTokensAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing updated Address Quantity to database when undoing spends";
                    }

                    if (!ptokensdb->WriteAddressTokenQuantity(undoSpend.address, undoSpend.tokenName,
                                                              mapTokensAddressAmount.at(pair))) {
                        dirty = true;
                        message = "_Failed Writing Address Balance to database";
                    }

                    if (dirty) {
                        return error("%s : %s", __func__, message);
                    }
                }
            }


            // Save the tokens that have been spent by erasing the quantity in the database
            for (auto spentToken : vSpentTokens) {
                auto pair = make_pair(spentToken.tokenName, spentToken.address);
                if (mapTokensAddressAmount.count(pair)) {
                    if (mapTokensAddressAmount.at(pair) == 0) {
                        if (!ptokensdb->EraseTokenAddressQuantity(spentToken.tokenName, spentToken.address)) {
                            dirty = true;
                            message = "_Failed Erasing a Spent Token, from database";
                        }

                        if (!ptokensdb->EraseAddressTokenQuantity(spentToken.address, spentToken.tokenName)) {
                            dirty = true;
                            message = "_Failed Erasing a Spent Token from AddressToken database";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    } else {
                        if (!ptokensdb->WriteTokenAddressQuantity(spentToken.tokenName, spentToken.address,
                                                                  mapTokensAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Erasing a Spent Token, from database";
                        }

                        if (!ptokensdb->WriteAddressTokenQuantity(spentToken.address, spentToken.tokenName,
                                                                  mapTokensAddressAmount.at(pair))) {
                            dirty = true;
                            message = "_Failed Writing Address Balance to database";
                        }

                        if (dirty) {
                            return error("%s : %s", __func__, message);
                        }
                    }
                }
            }
        }

        ClearDirtyCache();

        return true;
    } catch (const std::runtime_error& e) {
        return error("%s : %s ", __func__, std::string("System error while flushing tokens: ") + e.what());
    }
}

// This function will put all current cache data into the global ptokens cache.
//! Do not call this function on the ptokens pointer
bool CTokensCache::Flush()
{

    if (!ptokens)
        return error("%s: Couldn't find ptokens pointer while trying to flush tokens cache", __func__);

    try {
        for (auto &item : setNewTokensToAdd) {
            if (ptokens->setNewTokensToRemove.count(item))
                ptokens->setNewTokensToRemove.erase(item);
            ptokens->setNewTokensToAdd.insert(item);
        }

        for (auto &item : setNewTokensToRemove) {
            if (ptokens->setNewTokensToAdd.count(item))
                ptokens->setNewTokensToAdd.erase(item);
            ptokens->setNewTokensToRemove.insert(item);
        }

        for (auto &item : mapTokensAddressAmount)
            ptokens->mapTokensAddressAmount[item.first] = item.second;

        for (auto &item : mapReissuedTokenData)
            ptokens->mapReissuedTokenData[item.first] = item.second;

        for (auto &item : setNewOwnerTokensToAdd) {
            if (ptokens->setNewOwnerTokensToRemove.count(item))
                ptokens->setNewOwnerTokensToRemove.erase(item);
            ptokens->setNewOwnerTokensToAdd.insert(item);
        }

        for (auto &item : setNewOwnerTokensToRemove) {
            if (ptokens->setNewOwnerTokensToAdd.count(item))
                ptokens->setNewOwnerTokensToAdd.erase(item);
            ptokens->setNewOwnerTokensToRemove.insert(item);
        }

        for (auto &item : setNewReissueToAdd) {
            if (ptokens->setNewReissueToRemove.count(item))
                ptokens->setNewReissueToRemove.erase(item);
            ptokens->setNewReissueToAdd.insert(item);
        }

        for (auto &item : setNewReissueToRemove) {
            if (ptokens->setNewReissueToAdd.count(item))
                ptokens->setNewReissueToAdd.erase(item);
            ptokens->setNewReissueToRemove.insert(item);
        }

        for (auto &item : setNewTransferTokensToAdd) {
            if (ptokens->setNewTransferTokensToRemove.count(item))
                ptokens->setNewTransferTokensToRemove.erase(item);
            ptokens->setNewTransferTokensToAdd.insert(item);
        }

        for (auto &item : setNewTransferTokensToRemove) {
            if (ptokens->setNewTransferTokensToAdd.count(item))
                ptokens->setNewTransferTokensToAdd.erase(item);
            ptokens->setNewTransferTokensToRemove.insert(item);
        }

        for (auto &item : vSpentTokens) {
            ptokens->vSpentTokens.emplace_back(item);
        }

        for (auto &item : vUndoTokenAmount) {
            ptokens->vUndoTokenAmount.emplace_back(item);
        }

        return true;

    } catch (const std::runtime_error& e) {
        return error("%s : %s ", __func__, std::string("System error while flushing tokens: ") + e.what());
    }
}

//! Get the amount of memory the cache is using
size_t CTokensCache::DynamicMemoryUsage() const
{
    // TODO make sure this is accurate
    return memusage::DynamicUsage(mapTokensAddressAmount) + memusage::DynamicUsage(mapReissuedTokenData);
}

//! Get an estimated size of the cache in bytes that will be needed inorder to save to database
size_t CTokensCache::GetCacheSize() const
{
    // COutPoint: 32 bytes
    // CNewToken: Max 80 bytes
    // CTokenTransfer: Token Name, CAmount ( 40 bytes)
    // CReissueToken: Max 80 bytes
    // CAmount: 8 bytes
    // Token Name: Max 32 bytes
    // Address: 40 bytes
    // Block hash: 32 bytes
    // CTxOut: CAmount + CScript (105 + 8 = 113 bytes)

    size_t size = 0;

    size += (32 + 40 + 8) * vUndoTokenAmount.size(); // Token Name, Address, CAmount

    size += (40 + 40 + 32) * setNewTransferTokensToRemove.size(); // CTokenTrasnfer, Address, COutPoint
    size += (40 + 40 + 32) * setNewTransferTokensToAdd.size(); // CTokenTrasnfer, Address, COutPoint

    size += 72 * setNewOwnerTokensToAdd.size(); // Token Name, Address
    size += 72 * setNewOwnerTokensToRemove.size(); // Token Name, Address

    size += (32 + 40 + 8) * vSpentTokens.size(); // Token Name, Address, CAmount

    size += (80 + 40 + 32 + sizeof(int)) * setNewTokensToAdd.size(); // CNewToken, Address, Block hash, int
    size += (80 + 40 + 32 + sizeof(int)) * setNewTokensToRemove.size(); // CNewToken, Address, Block hash, int

    size += (80 + 40 + 32 + 32 + sizeof(int)) * setNewReissueToAdd.size(); // CReissueToken, Address, COutPoint, Block hash, int
    size += (80 + 40 + 32 + 32 + sizeof(int)) * setNewReissueToRemove.size(); // CReissueToken, Address, COutPoint, Block hash, int

    return size;
}

//! Get an estimated size of the cache in bytes that will be needed inorder to save to database
size_t CTokensCache::GetCacheSizeV2() const
{
    // COutPoint: 32 bytes
    // CNewToken: Max 80 bytes
    // CTokenTransfer: Token Name, CAmount ( 40 bytes)
    // CReissueToken: Max 80 bytes
    // CAmount: 8 bytes
    // Token Name: Max 32 bytes
    // Address: 40 bytes
    // Block hash: 32 bytes
    // CTxOut: CAmount + CScript (105 + 8 = 113 bytes)

    size_t size = 0;
    size += memusage::DynamicUsage(vUndoTokenAmount);
    size += memusage::DynamicUsage(setNewTransferTokensToRemove);
    size += memusage::DynamicUsage(setNewTransferTokensToAdd);
    size += memusage::DynamicUsage(setNewOwnerTokensToAdd);
    size += memusage::DynamicUsage(setNewOwnerTokensToRemove);
    size += memusage::DynamicUsage(vSpentTokens);
    size += memusage::DynamicUsage(setNewTokensToAdd);
    size += memusage::DynamicUsage(setNewTokensToRemove);
    size += memusage::DynamicUsage(setNewReissueToAdd);
    size += memusage::DynamicUsage(setNewReissueToRemove);

    return size;
}

// 1, 10, 100 ... COIN
// (0.00000001, 0.0000001, ... 1)
bool IsTokenUnitsValid(const CAmount& units)
{
    for (int i = 1; i <= COIN; i *= 10) {
        if (units == i) return true;
    }
    return false;
}

bool CheckIssueBurnTx(const CTxOut& txOut, const TokenType& type, const int numberIssued)
{
    CAmount burnAmount = 0;
    std::string burnAddress = "";

    if (type == TokenType::SUB) {
        burnAmount = GetIssueSubTokenBurnAmount();
        burnAddress = Params().IssueSubTokenBurnAddress();
    } else if (type == TokenType::ROOT) {
        burnAmount = GetIssueTokenBurnAmount();
        burnAddress = Params().IssueTokenBurnAddress();
    } else if (type == TokenType::UNIQUE) {
        burnAmount = GetIssueUniqueTokenBurnAmount();
        burnAddress = Params().IssueUniqueTokenBurnAddress();
    } else {
        return false;
    }

    // If issuing multiple (unique) tokens need to burn for each
    burnAmount *= numberIssued;

    // Check the first transaction for the required Burn Amount for the token type
    if (!(txOut.nValue == burnAmount))
        return false;

    // Extract the destination
    CTxDestination destination;
    if (!ExtractDestination(txOut.scriptPubKey, destination))
        return false;

    // Verify destination is valid
    if (!IsValidDestination(destination))
        return false;

    // Check destination address is the burn address
    auto strDestination = EncodeDestination(destination);
    if (!(strDestination == burnAddress))
        return false;

    return true;
}

bool CheckIssueBurnTx(const CTxOut& txOut, const TokenType& type)
{
    return CheckIssueBurnTx(txOut, type, 1);
}

bool CheckReissueBurnTx(const CTxOut& txOut)
{
    // Check the first transaction and verify that the correct ALP Amount
    if (txOut.nValue != GetReissueTokenBurnAmount())
        return false;

    // Extract the destination
    CTxDestination destination;
    if (!ExtractDestination(txOut.scriptPubKey, destination))
        return false;

    // Verify destination is valid
    if (!IsValidDestination(destination))
        return false;

    // Check destination address is the correct burn address
    if (EncodeDestination(destination) != Params().ReissueTokenBurnAddress())
        return false;

    return true;
}

bool CheckIssueDataTx(const CTxOut& txOut)
{
    // Verify 'ALPq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    int nStartingIndex = 0;
    return IsScriptNewToken(scriptPubKey, nStartingIndex);
}

bool CheckReissueDataTx(const CTxOut& txOut)
{
    // Verify 'ALPr' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptReissueToken(scriptPubKey);
}

bool CheckOwnerDataTx(const CTxOut& txOut)
{
    // Verify 'ALPq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptOwnerToken(scriptPubKey);
}

bool CheckTransferOwnerTx(const CTxOut& txOut)
{
    // Verify 'ALPq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptTransferToken(scriptPubKey);
}

bool IsScriptNewToken(const CScript& scriptPubKey)
{
    int index = 0;
    return IsScriptNewToken(scriptPubKey, index);
}

bool IsScriptNewToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_NEW_TOKEN && !fIsOwner;
    }
    return false;
}

bool IsScriptNewUniqueToken(const CScript& scriptPubKey)
{
    int index = 0;
    return IsScriptNewUniqueToken(scriptPubKey, index);
}

bool IsScriptNewUniqueToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner = false;
    if (!scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex))
        return false;

    CNewToken token;
    std::string address;
    if (!TokenFromScript(scriptPubKey, token, address))
        return false;

    TokenType tokenType;
    if (!IsTokenNameValid(token.strName, tokenType))
        return false;

    return TokenType::UNIQUE == tokenType;
}

bool IsScriptOwnerToken(const CScript& scriptPubKey)
{

    int index = 0;
    return IsScriptOwnerToken(scriptPubKey, index);
}

bool IsScriptOwnerToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_NEW_TOKEN && fIsOwner;
    }

    return false;
}

bool IsScriptReissueToken(const CScript& scriptPubKey)
{
    int index = 0;
    return IsScriptReissueToken(scriptPubKey, index);
}

bool IsScriptReissueToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_REISSUE_TOKEN;
    }

    return false;
}

bool IsScriptTransferToken(const CScript& scriptPubKey)
{
    int index = 0;
    return IsScriptTransferToken(scriptPubKey, index);
}

bool IsScriptTransferToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner = false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_TRANSFER_TOKEN;
    }

    return false;
}


//! Returns a boolean on if the token exists
bool CTokensCache::CheckIfTokenExists(const std::string& name, bool fForceDuplicateCheck)
{
    // TODO we need to add some Locks to this I would think

    // Create objects that will be used to check the dirty cache
    CNewToken token;
    token.strName = name;
    CTokenCacheNewToken cachedToken(token, "", 0, uint256());

    // Check the dirty caches first and see if it was recently added or removed
    if (setNewTokensToRemove.count(cachedToken))
        return false;

    // Check the dirty caches first and see if it was recently added or removed
    if (ptokens->setNewTokensToRemove.count(cachedToken))
        return false;

    if (setNewTokensToAdd.count(cachedToken)) {
        if (fForceDuplicateCheck)
            return true;
        else {
            LogPrintf("%s : Found token %s in setNewTokensToAdd but force duplicate check wasn't true\n", __func__, name);
        }
    }

    if (ptokens->setNewTokensToAdd.count(cachedToken)) {
        if (fForceDuplicateCheck)
            return true;
        else {
            LogPrintf("%s : Found token %s in setNewTokensToAdd but force duplicate check wasn't true\n", __func__, name);
        }
    }

    // Check the cache, if it doesn't exist in the cache. Try and read it from database
    if (ptokensCache) {
        if (ptokensCache->Exists(name)) {
            if (fForceDuplicateCheck)
                return true;
            else {
                LogPrintf("%s : Found token %s in ptokensCache but force duplicate check wasn't true\n", __func__, name);
            }
        } else {
            if (ptokensdb) {
                CNewToken readToken;
                int nHeight;
                uint256 hash;
                if (ptokensdb->ReadTokenData(name, readToken, nHeight, hash)) {
                    ptokensCache->Put(readToken.strName, CDatabasedTokenData(readToken, nHeight, hash));
                    if (fForceDuplicateCheck)
                        return true;
                    else {
                        LogPrintf("%s : Found token %s in ptokensdb but force duplicate check wasn't true\n", __func__, name);
                    }
                }
            }
        }
    }

    return false;
}

bool CTokensCache::GetTokenMetaDataIfExists(const std::string &name, CNewToken &token)
{
    int height;
    uint256 hash;
    return GetTokenMetaDataIfExists(name, token, height, hash);
}

bool CTokensCache::GetTokenMetaDataIfExists(const std::string &name, CNewToken &token, int& nHeight, uint256& blockHash)
{
    // Check the map that contains the reissued token data. If it is in this map, it hasn't been saved to disk yet
    if (mapReissuedTokenData.count(name)) {
        token = mapReissuedTokenData.at(name);
        return true;
    }

    // Check the map that contains the reissued token data. If it is in this map, it hasn't been saved to disk yet
    if (ptokens->mapReissuedTokenData.count(name)) {
        token = ptokens->mapReissuedTokenData.at(name);
        return true;
    }

    // Create objects that will be used to check the dirty cache
    CNewToken tempToken;
    tempToken.strName = name;
    CTokenCacheNewToken cachedToken(tempToken, "", 0, uint256());

    // Check the dirty caches first and see if it was recently added or removed
    if (setNewTokensToRemove.count(cachedToken)) {
        LogPrintf("%s : Found in new tokens to Remove - Returning False\n", __func__);
        return false;
    }

    // Check the dirty caches first and see if it was recently added or removed
    if (ptokens->setNewTokensToRemove.count(cachedToken)) {
        LogPrintf("%s : Found in new tokens to Remove - Returning False\n", __func__);
        return false;
    }

    auto setIterator = setNewTokensToAdd.find(cachedToken);
    if (setIterator != setNewTokensToAdd.end()) {
        token = setIterator->token;
        nHeight = setIterator->blockHeight;
        blockHash = setIterator->blockHash;
        return true;
    }

    setIterator = ptokens->setNewTokensToAdd.find(cachedToken);
    if (setIterator != ptokens->setNewTokensToAdd.end()) {
        token = setIterator->token;
        nHeight = setIterator->blockHeight;
        blockHash = setIterator->blockHash;
        return true;
    }

    // Check the cache, if it doesn't exist in the cache. Try and read it from database
    if (ptokensCache) {
        if (ptokensCache->Exists(name)) {
            CDatabasedTokenData data;
            data = ptokensCache->Get(name);
            token = data.token;
            nHeight = data.nHeight;
            blockHash = data.blockHash;
            return true;
        }
    }

    if (ptokensdb && ptokensCache) {
        CNewToken readToken;
        int height;
        uint256 hash;
        if (ptokensdb->ReadTokenData(name, readToken, height, hash)) {
            token = readToken;
            nHeight = height;
            blockHash = hash;
            ptokensCache->Put(readToken.strName, CDatabasedTokenData(readToken, height, hash));
            return true;
        }
    }

    LogPrintf("%s : Didn't find token meta data anywhere. Returning False\n", __func__);
    return false;
}

bool GetTokenInfoFromScript(const CScript& scriptPubKey, std::string& strName, CAmount& nAmount, uint32_t& nTokenLockTime)
{
    CTokenOutputEntry data;
    if(!GetTokenData(scriptPubKey, data))
        return false;

    strName = data.tokenName;
    nAmount = data.nAmount;
    nTokenLockTime = data.nTokenLockTime;

    return true;
}

bool GetTokenInfoFromCoin(const Coin& coin, std::string& strName, CAmount& nAmount, uint32_t& nTokenLockTime)
{
    return GetTokenInfoFromScript(coin.out.scriptPubKey, strName, nAmount, nTokenLockTime);
}

bool GetTokenData(const CScript& script, CTokenOutputEntry& data)
{
    // Placeholder strings that will get set if you successfully get the transfer or token from the script
    std::string address = "";
    std::string tokenName = "";

    int nType = 0;
    bool fIsOwner = false;
    if (!script.IsTokenScript(nType, fIsOwner)) {
        return false;
    }

    txnouttype type = txnouttype(nType);

    // Get the New Token or Transfer Token from the scriptPubKey
    if (type == TX_NEW_TOKEN && !fIsOwner) {
        CNewToken token;
        if (TokenFromScript(script, token, address)) {
            data.type = TX_NEW_TOKEN;
            data.nAmount = token.nAmount;
            data.destination = DecodeDestination(address);
            data.tokenName = token.strName;
            data.nTokenLockTime = 0;
            return true;
        }
    } else if (type == TX_TRANSFER_TOKEN) {
        CTokenTransfer transfer;
        if (TransferTokenFromScript(script, transfer, address)) {
            data.type = TX_TRANSFER_TOKEN;
            data.nAmount = transfer.nAmount;
            data.destination = DecodeDestination(address);
            data.tokenName = transfer.strName;
            data.nTokenLockTime = transfer.nTokenLockTime;
            return true;
        }
    } else if (type == TX_NEW_TOKEN && fIsOwner) {
        if (OwnerTokenFromScript(script, tokenName, address)) {
            data.type = TX_NEW_TOKEN;
            data.nAmount = OWNER_TOKEN_AMOUNT;
            data.destination = DecodeDestination(address);
            data.tokenName = tokenName;
            data.nTokenLockTime = 0;
            return true;
        }
    } else if (type == TX_REISSUE_TOKEN) {
        CReissueToken reissue;
        if (ReissueTokenFromScript(script, reissue, address)) {
            data.type = TX_REISSUE_TOKEN;
            data.nAmount = reissue.nAmount;
            data.destination = DecodeDestination(address);
            data.tokenName = reissue.strName;
            data.nTokenLockTime = 0;
            return true;
        }
    }

    return false;
}

void GetAllAdministrativeTokens(CWallet *pwallet, std::vector<std::string> &names, int nMinConf)
{
    if(!pwallet)
        return;

    GetAllMyTokens(pwallet, names, nMinConf, true, true);
}

void GetAllMyTokens(CWallet* pwallet, std::vector<std::string>& names, int nMinConf, bool fIncludeAdministrator, bool fOnlyAdministrator)
{
    if(!pwallet)
        return;

    std::map<std::string, std::vector<COutput> > mapTokens;
    pwallet->AvailableTokens(mapTokens, true, nullptr, 1, MAX_MONEY_TOKENS, MAX_MONEY_TOKENS, 0, nMinConf); // Set the mincof, set the rest to the defaults

    for (auto item : mapTokens) {
        bool isOwner = IsTokenNameAnOwner(item.first);

        if (isOwner) {
            if (fOnlyAdministrator || fIncludeAdministrator)
                names.emplace_back(item.first);
        } else {
            if (fOnlyAdministrator)
                continue;
            names.emplace_back(item.first);
        }
    }
}

CAmount GetIssueTokenBurnAmount()
{
    return Params().MainFeeAmount();
}

CAmount GetReissueTokenBurnAmount()
{
    return Params().SecondaryFeeAmount();
}

CAmount GetIssueSubTokenBurnAmount()
{
    return Params().SecondaryFeeAmount();
}

CAmount GetIssueUniqueTokenBurnAmount()
{
    return Params().SecondaryFeeAmount();
}

CAmount GetBurnAmount(const int nType)
{
    return GetBurnAmount((TokenType(nType)));
}

CAmount GetBurnAmount(const TokenType type)
{
    switch (type) {
        case TokenType::ROOT:
            return GetIssueTokenBurnAmount();
        case TokenType::SUB:
            return GetIssueSubTokenBurnAmount();
        case TokenType::MSGCHANNEL:
            return 0;
        case TokenType::OWNER:
            return 0;
        case TokenType::UNIQUE:
            return GetIssueUniqueTokenBurnAmount();
        case TokenType::VOTE:
            return 0;
        case TokenType::REISSUE:
            return GetReissueTokenBurnAmount();
        default:
            return 0;
    }
}

std::string GetBurnAddress(const int nType)
{
    return GetBurnAddress((TokenType(nType)));
}

std::string GetBurnAddress(const TokenType type)
{
    switch (type) {
        case TokenType::ROOT:
            return Params().IssueTokenBurnAddress();
        case TokenType::SUB:
            return Params().IssueSubTokenBurnAddress();
        case TokenType::MSGCHANNEL:
            return "";
        case TokenType::OWNER:
            return "";
        case TokenType::UNIQUE:
            return Params().IssueUniqueTokenBurnAddress();
        case TokenType::VOTE:
            return "";
        case TokenType::REISSUE:
            return Params().ReissueTokenBurnAddress();
        default:
            return "";
    }
}

//! This will get the amount that an address for a certain token contains from the database if they cache doesn't already have it
bool GetBestTokenAddressAmount(CTokensCache& cache, const std::string& tokenName, const std::string& address)
{
    if (fTokenIndex) {
        auto pair = make_pair(tokenName, address);

        // If the caches map has the pair, return true because the map already contains the best dirty amount
        if (cache.mapTokensAddressAmount.count(pair))
            return true;

        // If the caches map has the pair, return true because the map already contains the best dirty amount
        if (ptokens->mapTokensAddressAmount.count(pair)) {
            cache.mapTokensAddressAmount[pair] = ptokens->mapTokensAddressAmount.at(pair);
            return true;
        }

        // If the database contains the tokens address amount, insert it into the database and return true
        CAmount nDBAmount;
        if (ptokensdb->ReadTokenAddressQuantity(pair.first, pair.second, nDBAmount)) {
            cache.mapTokensAddressAmount.insert(make_pair(pair, nDBAmount));
            return true;
        }
    }

    // The amount wasn't found return false
    return false;
}

//! sets _balances_ with the total quantity of each owned token
bool GetAllMyTokenBalances(std::map<std::string, std::vector<COutput> >& outputs, std::map<std::string, CAmount>& amounts, const std::string& prefix) {

    // Return false if no wallet was found to compute token balances
    if (!vpwallets.size())
        return false;

    // Get the map of tokennames to outputs
    vpwallets[0]->AvailableTokens(outputs, true, nullptr, 1, MAX_MONEY_TOKENS, MAX_MONEY_TOKENS);

    // Loop through all pairs of Token Name -> vector<COutput>
    for (const auto& pair : outputs) {
        if (prefix.empty() || pair.first.find(prefix) == 0) { // Check for prefix
            CAmount balance = 0;
            for (auto txout : pair.second) { // Compute balance of token by summing all Available Outputs
                CTokenOutputEntry data;
                if (GetTokenData(txout.tx->tx->vout[txout.i].scriptPubKey, data))
                    balance += data.nAmount;
            }
            amounts.insert(std::make_pair(pair.first, balance));
        }
    }

    return true;
}

//! sets _balances_ with the total quantity of each owned locked token
bool GetAllMyLockedTokenBalances(std::map<std::string, std::vector<COutput> >& outputs, std::map<std::string, CAmount>& amounts, const std::string& prefix) {

    // Return false if no wallet was found to compute token balances
    if (!vpwallets.size())
        return false;

    // Get the map of tokennames to outputs
    vpwallets[0]->LockedTokens(outputs, true, nullptr, 1, MAX_MONEY_TOKENS, MAX_MONEY_TOKENS);

    // Loop through all pairs of Token Name -> vector<COutput>
    for (const auto& pair : outputs) {
        if (prefix.empty() || pair.first.find(prefix) == 0) { // Check for prefix
            CAmount balance = 0;
            for (auto txout : pair.second) { // Compute balance of token by summing all Available Outputs
                CTokenOutputEntry data;
                if (GetTokenData(txout.tx->tx->vout[txout.i].scriptPubKey, data))
                    balance += data.nAmount;
            }
            amounts.insert(std::make_pair(pair.first, balance));
        }
    }

    return true;
}

// 46 char base58 --> 34 char KAW compatible
std::string DecodeIPFS(std::string encoded)
{
    std::vector<unsigned char> b;
    DecodeBase58(encoded, b);
    return std::string(b.begin(), b.end());
};

// 34 char KAW compatible --> 46 char base58
std::string EncodeIPFS(std::string decoded){
    std::vector<char> charData(decoded.begin(), decoded.end());
    std::vector<unsigned char> unsignedCharData;
    for (char c : charData)
        unsignedCharData.push_back(static_cast<unsigned char>(c));
    return EncodeBase58(unsignedCharData);
};

bool CreateTokenTransaction(CWallet* pwallet, CCoinControl& coinControl, const CNewToken& token, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    std::vector<CNewToken> tokens;
    tokens.push_back(token);
    return CreateTokenTransaction(pwallet, coinControl, tokens, address, error, wtxNew, reservekey, nFeeRequired);
}

bool CreateTokenTransaction(CWallet* pwallet, CCoinControl& coinControl, const std::vector<CNewToken> tokens, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    std::string change_address = EncodeDestination(coinControl.destChange);

    auto currentActiveTokenCache = GetCurrentTokenCache();
    // Validate the tokens data
    std::string strError;
    for (auto token : tokens) {
        if (!token.IsValid(strError, *currentActiveTokenCache)) {
            error = std::make_pair(RPC_INVALID_PARAMETER, strError);
            return false;
        }
    }

    if (!change_address.empty()) {
        CTxDestination destination = DecodeDestination(change_address);
        if (!IsValidDestination(destination)) {
            error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Alphacon address: ") + change_address);
            return false;
        }
    } else {
        // no coin control: send change to newly generated address
        CKeyID keyID;
        std::string strFailReason;
        if (!pwallet->CreateNewChangeAddress(reservekey, keyID, strFailReason)) {
            error = std::make_pair(RPC_WALLET_KEYPOOL_RAN_OUT, strFailReason);
            return false;
        }

        change_address = EncodeDestination(keyID);
        coinControl.destChange = DecodeDestination(change_address);
    }

    TokenType tokenType;
    std::string parentName;
    for (auto token : tokens) {
        if (!IsTokenNameValid(token.strName, tokenType)) {
            error = std::make_pair(RPC_INVALID_PARAMETER, "Token name not valid");
            return false;
        }
        if (tokens.size() > 1 && tokenType != TokenType::UNIQUE) {
            error = std::make_pair(RPC_INVALID_PARAMETER, "Only unique tokens can be issued in bulk.");
            return false;
        }
        std::string parent = GetParentName(token.strName);
        if (parentName.empty())
            parentName = parent;
        if (parentName != parent) {
            error = std::make_pair(RPC_INVALID_PARAMETER, "All tokens must have the same parent.");
            return false;
        }
    }

    // Assign the correct burn amount and the correct burn address depending on the type of token issuance that is happening
    CAmount burnAmount = GetBurnAmount(tokenType) * tokens.size();
    CScript scriptPubKey = GetScriptForDestination(DecodeDestination(GetBurnAddress(tokenType)));

    CAmount curBalance = pwallet->GetBalance();

    // Check to make sure the wallet has the ALP required by the burnAmount
    if (curBalance < burnAmount) {
        error = std::make_pair(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        return false;
    }

    if (pwallet->GetBroadcastTransactions() && !g_connman) {
        error = std::make_pair(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
        return false;
    }

    LOCK2(cs_main, pwallet->cs_wallet);

    // Create and send the transaction
    std::string strTxError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    bool fSubtractFeeFromAmount = false;

    CRecipient recipient = {scriptPubKey, burnAmount, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);

    // If the token is a subtoken or unique token. We need to send the ownertoken change back to ourselfs
    if (tokenType == TokenType::SUB || tokenType == TokenType::UNIQUE) {
        // Get the script for the destination address for the tokens
        CScript scriptTransferOwnerToken = GetScriptForDestination(DecodeDestination(change_address));

        CTokenTransfer tokenTransfer(parentName + OWNER_TAG, OWNER_TOKEN_AMOUNT, 0);
        tokenTransfer.ConstructTransaction(scriptTransferOwnerToken);
        CRecipient rec = {scriptTransferOwnerToken, 0, fSubtractFeeFromAmount};
        vecSend.push_back(rec);
    }

    // Get the owner outpoints if this is a subtoken or unique token
    if (tokenType == TokenType::SUB || tokenType == TokenType::UNIQUE) {
        // Verify that this wallet is the owner for the token, and get the owner token outpoint
        for (auto token : tokens) {
            if (!VerifyWalletHasToken(parentName + OWNER_TAG, error)) {
                return false;
            }
        }
    }

    if (!pwallet->CreateTransactionWithTokens(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strTxError, coinControl, tokens, DecodeDestination(address), tokenType)) {
        if (!fSubtractFeeFromAmount && burnAmount + nFeeRequired > curBalance)
            strTxError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        error = std::make_pair(RPC_WALLET_ERROR, strTxError);
        return false;
    }
    return true;
}

bool CreateReissueTokenTransaction(CWallet* pwallet, CCoinControl& coinControl, const CReissueToken& reissueToken, const std::string& address, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    std::string token_name = reissueToken.strName;
    std::string change_address = EncodeDestination(coinControl.destChange);

    // Check that validitity of the address
    if (!IsValidDestinationString(address)) {
        error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Alphacon address: ") + address);
        return false;
    }

    if (!change_address.empty()) {
        CTxDestination destination = DecodeDestination(change_address);
        if (!IsValidDestination(destination)) {
            error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Alphacon address: ") + change_address);
            return false;
        }
    } else {
        CKeyID keyID;
        std::string strFailReason;
        if (!pwallet->CreateNewChangeAddress(reservekey, keyID, strFailReason)) {
            error = std::make_pair(RPC_WALLET_KEYPOOL_RAN_OUT, strFailReason);
            return false;
        }

        change_address = EncodeDestination(keyID);
        coinControl.destChange = DecodeDestination(change_address);
    }

    // Check the tokens name
    if (!IsTokenNameValid(token_name)) {
        error = std::make_pair(RPC_INVALID_PARAMS, std::string("Invalid token name: ") + token_name);
        return false;
    }

    if (IsTokenNameAnOwner(token_name)) {
        error = std::make_pair(RPC_INVALID_PARAMS, std::string("Owner Tokens are not able to be reissued"));
        return false;
    }

    // ptokens and ptokensCache need to be initialized
    auto currentActiveTokenCache = GetCurrentTokenCache();
    if (!currentActiveTokenCache) {
        error = std::make_pair(RPC_DATABASE_ERROR, std::string("ptokens isn't initialized"));
        return false;
    }

    if (!ptokensCache) {
        error = std::make_pair(RPC_DATABASE_ERROR,
                               std::string("ptokensCache isn't initialized"));
        return false;
    }

    std::string strError;
    if (!reissueToken.IsValid(strError, *currentActiveTokenCache)) {
        error = std::make_pair(RPC_VERIFY_ERROR,
                               std::string("Failed to create reissue token object. Error: ") + strError);
        return false;
    }

    // Verify that this wallet is the owner for the token, and get the owner token outpoint
    if (!VerifyWalletHasToken(token_name + OWNER_TAG, error)) {
        return false;
    }

    // Check the wallet balance
    CAmount curBalance = pwallet->GetBalance();

    // Get the current burn amount for issuing an token
    CAmount burnAmount = GetReissueTokenBurnAmount();

    // Check to make sure the wallet has the ALP required by the burnAmount
    if (curBalance < burnAmount) {
        error = std::make_pair(RPC_WALLET_INSUFFICIENT_FUNDS, "Insufficient funds");
        return false;
    }

    if (pwallet->GetBroadcastTransactions() && !g_connman) {
        error = std::make_pair(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
        return false;
    }

    // Get the script for the destination address for the tokens
    CScript scriptTransferOwnerToken = GetScriptForDestination(DecodeDestination(change_address));

    CTokenTransfer tokenTransfer(token_name + OWNER_TAG, OWNER_TOKEN_AMOUNT, 0);
    tokenTransfer.ConstructTransaction(scriptTransferOwnerToken);

    // Get the script for the burn address
    CScript scriptPubKeyBurn = GetScriptForDestination(DecodeDestination(Params().ReissueTokenBurnAddress()));

    // Create and send the transaction
    std::string strTxError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    bool fSubtractFeeFromAmount = false;
    CRecipient recipient = {scriptPubKeyBurn, burnAmount, fSubtractFeeFromAmount};
    CRecipient recipient2 = {scriptTransferOwnerToken, 0, fSubtractFeeFromAmount};
    vecSend.push_back(recipient);
    vecSend.push_back(recipient2);
    if (!pwallet->CreateTransactionWithReissueToken(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strTxError, coinControl, reissueToken, DecodeDestination(address))) {
        if (!fSubtractFeeFromAmount && burnAmount + nFeeRequired > curBalance)
            strTxError = strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired));
        error = std::make_pair(RPC_WALLET_ERROR, strTxError);
        return false;
    }
    return true;
}

bool CreateTransferTokenTransaction(CWallet* pwallet, const CCoinControl& coinControl, const std::vector< std::pair<CTokenTransfer, std::string> >vTransfers, const std::string& changeAddress, std::pair<int, std::string>& error, CWalletTx& wtxNew, CReserveKey& reservekey, CAmount& nFeeRequired)
{
    // Initialize Values for transaction
    std::string strTxError;
    std::vector<CRecipient> vecSend;
    int nChangePosRet = -1;
    bool fSubtractFeeFromAmount = false;

    // Check for a balance before processing transfers
    CAmount curBalance = pwallet->GetBalance();
    if (curBalance == 0) {
        error = std::make_pair(RPC_WALLET_INSUFFICIENT_FUNDS, std::string("This wallet doesn't contain any ALP, transfering an token requires a network fee"));
        return false;
    }

    // Check for peers and connections
    if (pwallet->GetBroadcastTransactions() && !g_connman) {
        error = std::make_pair(RPC_CLIENT_P2P_DISABLED, "Error: Peer-to-peer functionality missing or disabled");
        return false;
    }

    // Loop through all transfers and create scriptpubkeys for them
    for (auto transfer : vTransfers) {
        std::string address = transfer.second;
        std::string token_name = transfer.first.strName;
        CAmount nAmount = transfer.first.nAmount;
        uint32_t nTokenLockTime = transfer.first.nTokenLockTime;

        if (!IsValidDestinationString(address)) {
            error = std::make_pair(RPC_INVALID_ADDRESS_OR_KEY, std::string("Invalid Alphacon address: ") + address);
            return false;
        }
        auto currentActiveTokenCache = GetCurrentTokenCache();
        if (!currentActiveTokenCache) {
            error = std::make_pair(RPC_DATABASE_ERROR, std::string("ptokens isn't initialized"));
            return false;
        }

        if (!VerifyWalletHasToken(token_name, error)) // Sets error if it fails
            return false;

        // If it is an ownership transfer, make a quick check to make sure the amount is 1
        if (IsTokenNameAnOwner(token_name)) {
            if (nAmount != OWNER_TOKEN_AMOUNT) {
                error = std::make_pair(RPC_INVALID_PARAMS, std::string(
                        "When transfer an 'Ownership Token' the amount must always be 1. Please try again with the amount of 1"));
                return false;
            }
        }

        // Get the script for the burn address
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(address));

        // Update the scriptPubKey with the transfer token information
        CTokenTransfer tokenTransfer(token_name, nAmount, nTokenLockTime);
        tokenTransfer.ConstructTransaction(scriptPubKey);

        CRecipient recipient = {scriptPubKey, 0, fSubtractFeeFromAmount};
        vecSend.push_back(recipient);
    }

    // Create and send the transaction
    if (!pwallet->CreateTransactionWithTransferToken(vecSend, wtxNew, reservekey, nFeeRequired, nChangePosRet, strTxError, coinControl)) {
        if (!fSubtractFeeFromAmount && nFeeRequired > curBalance) {
            error = std::make_pair(RPC_WALLET_ERROR, strprintf("Error: This transaction requires a transaction fee of at least %s", FormatMoney(nFeeRequired)));
            return false;
        }
        error = std::make_pair(RPC_TRANSACTION_ERROR, strTxError);
        return false;
    }
    return true;
}

bool SendTokenTransaction(CWallet* pwallet, CWalletTx& transaction, CReserveKey& reserveKey, std::pair<int, std::string>& error, std::string& txid)
{
    CValidationState state;
    if (!pwallet->CommitTransaction(transaction, reserveKey, g_connman.get(), state)) {
        error = std::make_pair(RPC_WALLET_ERROR, strprintf("Error: The transaction was rejected! Reason given: %s", state.GetRejectReason()));
        return false;
    }

    txid = transaction.GetHash().GetHex();
    return true;
}

bool VerifyWalletHasToken(const std::string& token_name, std::pair<int, std::string>& pairError)
{
    CWallet* pwallet;
    if (vpwallets.size() > 0)
        pwallet = vpwallets[0];
    else {
        pairError = std::make_pair(RPC_WALLET_ERROR, strprintf("Wallet not found. Can't verify if it contains: %s", token_name));
        return false;
    }

    std::vector<COutput> vCoins;
    std::map<std::string, std::vector<COutput> > mapTokenCoins;
    pwallet->AvailableTokens(mapTokenCoins);

    if (mapTokenCoins.count(token_name))
        return true;

    pairError = std::make_pair(RPC_INVALID_REQUEST, strprintf("Wallet doesn't have token: %s", token_name));
    return false;
}

// Return true if the amount is valid with the units passed in
bool CheckAmountWithUnits(const CAmount& nAmount, const int8_t nUnits)
{
    return nAmount % int64_t(pow(10, (MAX_UNIT - nUnits))) == 0;
}

bool CheckEncodedIPFS(const std::string& hash, std::string& strError)
{
    if (hash.substr(0, 2) != "Qm") {
        strError = _("Invalid parameter: ipfs_hash must start with 'Qm'.");
        return false;
    }

    return true;
}

void GetTxOutTokenTypes(const std::vector<CTxOut>& vout, int& issues, int& reissues, int& transfers, int& owners)
{
    for (auto out: vout) {
        int type;
        bool fIsOwner;
        if (out.scriptPubKey.IsTokenScript(type, fIsOwner)) {
            if (type == TX_NEW_TOKEN && !fIsOwner)
                issues++;
            else if (type == TX_NEW_TOKEN && fIsOwner)
                owners++;
            else if (type == TX_TRANSFER_TOKEN)
                transfers++;
            else if (type == TX_REISSUE_TOKEN)
                reissues++;
        }
    }
}

bool ParseTokenScript(CScript scriptPubKey, uint160 &hashBytes, std::string &tokenName, CAmount &tokenAmount) {
    int nType;
    bool fIsOwner;
    int _nStartingPoint;
    std::string _strAddress;
    bool isToken = false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, _nStartingPoint)) {
        if (nType == TX_NEW_TOKEN) {
            if (fIsOwner) {
                if (OwnerTokenFromScript(scriptPubKey, tokenName, _strAddress)) {
                    tokenAmount = OWNER_TOKEN_AMOUNT;
                    isToken = true;
                } else {
                    LogPrintf("%s : Couldn't get new owner token from script: %s", __func__, HexStr(scriptPubKey));
                }
            } else {
                CNewToken token;
                if (TokenFromScript(scriptPubKey, token, _strAddress)) {
                    tokenName = token.strName;
                    tokenAmount = token.nAmount;
                    isToken = true;
                } else {
                    LogPrintf("%s : Couldn't get new token from script: %s", __func__, HexStr(scriptPubKey));
                }
            }
        } else if (nType == TX_REISSUE_TOKEN) {
            CReissueToken token;
            if (ReissueTokenFromScript(scriptPubKey, token, _strAddress)) {
                tokenName = token.strName;
                tokenAmount = token.nAmount;
                isToken = true;
            } else {
                LogPrintf("%s : Couldn't get reissue token from script: %s", __func__, HexStr(scriptPubKey));
            }
        } else if (nType == TX_TRANSFER_TOKEN) {
            CTokenTransfer token;
            if (TransferTokenFromScript(scriptPubKey, token, _strAddress)) {
                tokenName = token.strName;
                tokenAmount = token.nAmount;
                isToken = true;
            } else {
                LogPrintf("%s : Couldn't get transfer token from script: %s", __func__, HexStr(scriptPubKey));
            }
        } else {
            LogPrintf("%s : Unsupported token type: %s", __func__, nType);
        }
    } else {
//        LogPrintf("%s : Found no token in script: %s", __func__, HexStr(scriptPubKey));
    }
    if (isToken) {
//        LogPrintf("%s : Found tokens in script at address %s : %s (%s)", __func__, _strAddress, tokenName, tokenAmount);
        hashBytes = uint160(std::vector <unsigned char>(scriptPubKey.begin()+3, scriptPubKey.begin()+23));
        return true;
    }
    return false;
}