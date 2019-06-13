// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2014 The BlackCoin developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "streams.h"
#include "version.h"
#include "tokens/tokens.h"
#include "script.h"

#include "tinyformat.h"
#include "utilstrencodings.h"
#include "standard.h"

const char* GetOpName(opcodetype opcode)
{
    switch (opcode)
    {
    // push value
    case OP_0                      : return "0";
    case OP_PUSHDATA1              : return "OP_PUSHDATA1";
    case OP_PUSHDATA2              : return "OP_PUSHDATA2";
    case OP_PUSHDATA4              : return "OP_PUSHDATA4";
    case OP_1NEGATE                : return "-1";
    case OP_RESERVED               : return "OP_RESERVED";
    case OP_1                      : return "1";
    case OP_2                      : return "2";
    case OP_3                      : return "3";
    case OP_4                      : return "4";
    case OP_5                      : return "5";
    case OP_6                      : return "6";
    case OP_7                      : return "7";
    case OP_8                      : return "8";
    case OP_9                      : return "9";
    case OP_10                     : return "10";
    case OP_11                     : return "11";
    case OP_12                     : return "12";
    case OP_13                     : return "13";
    case OP_14                     : return "14";
    case OP_15                     : return "15";
    case OP_16                     : return "16";

    // control
    case OP_NOP                    : return "OP_NOP";
    case OP_VER                    : return "OP_VER";
    case OP_IF                     : return "OP_IF";
    case OP_NOTIF                  : return "OP_NOTIF";
    case OP_VERIF                  : return "OP_VERIF";
    case OP_VERNOTIF               : return "OP_VERNOTIF";
    case OP_ELSE                   : return "OP_ELSE";
    case OP_ENDIF                  : return "OP_ENDIF";
    case OP_VERIFY                 : return "OP_VERIFY";
    case OP_RETURN                 : return "OP_RETURN";

    // stack ops
    case OP_TOALTSTACK             : return "OP_TOALTSTACK";
    case OP_FROMALTSTACK           : return "OP_FROMALTSTACK";
    case OP_2DROP                  : return "OP_2DROP";
    case OP_2DUP                   : return "OP_2DUP";
    case OP_3DUP                   : return "OP_3DUP";
    case OP_2OVER                  : return "OP_2OVER";
    case OP_2ROT                   : return "OP_2ROT";
    case OP_2SWAP                  : return "OP_2SWAP";
    case OP_IFDUP                  : return "OP_IFDUP";
    case OP_DEPTH                  : return "OP_DEPTH";
    case OP_DROP                   : return "OP_DROP";
    case OP_DUP                    : return "OP_DUP";
    case OP_NIP                    : return "OP_NIP";
    case OP_OVER                   : return "OP_OVER";
    case OP_PICK                   : return "OP_PICK";
    case OP_ROLL                   : return "OP_ROLL";
    case OP_ROT                    : return "OP_ROT";
    case OP_SWAP                   : return "OP_SWAP";
    case OP_TUCK                   : return "OP_TUCK";

    // splice ops
    case OP_CAT                    : return "OP_CAT";
    case OP_SUBSTR                 : return "OP_SUBSTR";
    case OP_LEFT                   : return "OP_LEFT";
    case OP_RIGHT                  : return "OP_RIGHT";
    case OP_SIZE                   : return "OP_SIZE";

    // bit logic
    case OP_INVERT                 : return "OP_INVERT";
    case OP_AND                    : return "OP_AND";
    case OP_OR                     : return "OP_OR";
    case OP_XOR                    : return "OP_XOR";
    case OP_EQUAL                  : return "OP_EQUAL";
    case OP_EQUALVERIFY            : return "OP_EQUALVERIFY";
    case OP_RESERVED1              : return "OP_RESERVED1";
    case OP_RESERVED2              : return "OP_RESERVED2";

    // numeric
    case OP_1ADD                   : return "OP_1ADD";
    case OP_1SUB                   : return "OP_1SUB";
    case OP_2MUL                   : return "OP_2MUL";
    case OP_2DIV                   : return "OP_2DIV";
    case OP_NEGATE                 : return "OP_NEGATE";
    case OP_ABS                    : return "OP_ABS";
    case OP_NOT                    : return "OP_NOT";
    case OP_0NOTEQUAL              : return "OP_0NOTEQUAL";
    case OP_ADD                    : return "OP_ADD";
    case OP_SUB                    : return "OP_SUB";
    case OP_MUL                    : return "OP_MUL";
    case OP_DIV                    : return "OP_DIV";
    case OP_MOD                    : return "OP_MOD";
    case OP_LSHIFT                 : return "OP_LSHIFT";
    case OP_RSHIFT                 : return "OP_RSHIFT";
    case OP_BOOLAND                : return "OP_BOOLAND";
    case OP_BOOLOR                 : return "OP_BOOLOR";
    case OP_NUMEQUAL               : return "OP_NUMEQUAL";
    case OP_NUMEQUALVERIFY         : return "OP_NUMEQUALVERIFY";
    case OP_NUMNOTEQUAL            : return "OP_NUMNOTEQUAL";
    case OP_LESSTHAN               : return "OP_LESSTHAN";
    case OP_GREATERTHAN            : return "OP_GREATERTHAN";
    case OP_LESSTHANOREQUAL        : return "OP_LESSTHANOREQUAL";
    case OP_GREATERTHANOREQUAL     : return "OP_GREATERTHANOREQUAL";
    case OP_MIN                    : return "OP_MIN";
    case OP_MAX                    : return "OP_MAX";
    case OP_WITHIN                 : return "OP_WITHIN";

    // crypto
    case OP_RIPEMD160              : return "OP_RIPEMD160";
    case OP_SHA1                   : return "OP_SHA1";
    case OP_SHA256                 : return "OP_SHA256";
    case OP_HASH160                : return "OP_HASH160";
    case OP_HASH256                : return "OP_HASH256";
    case OP_CODESEPARATOR          : return "OP_CODESEPARATOR";
    case OP_CHECKSIG               : return "OP_CHECKSIG";
    case OP_CHECKSIGVERIFY         : return "OP_CHECKSIGVERIFY";
    case OP_CHECKMULTISIG          : return "OP_CHECKMULTISIG";
    case OP_CHECKMULTISIGVERIFY    : return "OP_CHECKMULTISIGVERIFY";

    // expansion
    case OP_NOP1                   : return "OP_NOP1";
    case OP_CHECKLOCKTIMEVERIFY    : return "OP_CHECKLOCKTIMEVERIFY";
    case OP_CHECKSEQUENCEVERIFY    : return "OP_CHECKSEQUENCEVERIFY";
    case OP_NOP4                   : return "OP_NOP4";
    case OP_NOP5                   : return "OP_NOP5";
    case OP_NOP6                   : return "OP_NOP6";
    case OP_NOP7                   : return "OP_NOP7";
    case OP_NOP8                   : return "OP_NOP8";
    case OP_NOP9                   : return "OP_NOP9";
    case OP_NOP10                  : return "OP_NOP10";

    /** TOKENS START */
    case OP_ALP_TOKEN              : return "OP_ALP_TOKEN";
    /** TOKENS END */

    case OP_INVALIDOPCODE          : return "OP_INVALIDOPCODE";

    // Note:
    //  The template matching params OP_SMALLINTEGER/etc are defined in opcodetype enum
    //  as kind of implementation hack, they are *NOT* real opcodes.  If found in real
    //  Script, just let the default: case deal with them.

    default:
        return "OP_UNKNOWN";
    }
}

unsigned int CScript::GetSigOpCount(bool fAccurate) const
{
    unsigned int n = 0;
    const_iterator pc = begin();
    opcodetype lastOpcode = OP_INVALIDOPCODE;
    while (pc < end())
    {
        opcodetype opcode;
        if (!GetOp(pc, opcode))
            break;
        if (opcode == OP_CHECKSIG || opcode == OP_CHECKSIGVERIFY)
            n++;
        else if (opcode == OP_CHECKMULTISIG || opcode == OP_CHECKMULTISIGVERIFY)
        {
            if (fAccurate && lastOpcode >= OP_1 && lastOpcode <= OP_16)
                n += DecodeOP_N(lastOpcode);
            else
                n += MAX_PUBKEYS_PER_MULTISIG;
        }
        lastOpcode = opcode;
    }
    return n;
}

unsigned int CScript::GetSigOpCount(const CScript& scriptSig) const
{
    if (!IsPayToScriptHash())
        return GetSigOpCount(true);

    // This is a pay-to-script-hash scriptPubKey;
    // get the last item that the scriptSig
    // pushes onto the stack:
    const_iterator pc = scriptSig.begin();
    std::vector<unsigned char> vData;
    while (pc < scriptSig.end())
    {
        opcodetype opcode;
        if (!scriptSig.GetOp(pc, opcode, vData))
            return 0;
        if (opcode > OP_16)
            return 0;
    }

    /// ... and return its opcount:
    CScript subscript(vData.begin(), vData.end());
    return subscript.GetSigOpCount(true);
}

bool CScript::IsPayToPublicKeyHash() const
{
    // Extra-fast test for pay-to-pubkey-hash CScripts:
    return (this->size() == 25 &&
	    (*this)[0] == OP_DUP &&
	    (*this)[1] == OP_HASH160 &&
	    (*this)[2] == 0x14 &&
	    (*this)[23] == OP_EQUALVERIFY &&
	    (*this)[24] == OP_CHECKSIG);
}

bool CScript::IsPayToScriptHash() const
{
    // Extra-fast test for pay-to-script-hash CScripts:
    return (this->size() == 23 &&
            (*this)[0] == OP_HASH160 &&
            (*this)[1] == 0x14 &&
            (*this)[22] == OP_EQUAL);
}

/** TOKENS START */
bool CScript::IsTokenScript() const
{
    int nType = 0;
    bool isOwner = false;
    int start = 0;
    return IsTokenScript(nType, isOwner, start);
}

bool CScript::IsTokenScript(int& nType, bool& isOwner) const
{
    int start = 0;
    return IsTokenScript(nType, isOwner, start);
}

bool CScript::IsTokenScript(int& nType, bool& fIsOwner, int& nStartingIndex) const
{
    if (this->size() > 30) {
        if ((*this)[25] == OP_ALP_TOKEN) { // OP_ALP_TOKEN is always in the 25 index of the script if it exists
            int index = -1;
            if ((*this)[27] == ALP_A) { // Check to see if ALP starts at 27 ( this->size() < 105)
                if ((*this)[28] == ALP_L)
                    if ((*this)[29] == ALP_P)
                        index = 30;
            } else {
                if ((*this)[28] == ALP_A) // Check to see if ALP starts at 28 ( this->size() >= 105)
                    if ((*this)[29] == ALP_L)
                        if ((*this)[30] == ALP_P)
                            index = 31;
            }

            if (index > 0) {
                nStartingIndex = index + 1; // Set the index where the token data begins. Use to serialize the token data into token objects
                if ((*this)[index] == ALP_T) { // Transfer first anticipating more transfers than other tokens operations
                    nType = TX_TRANSFER_TOKEN;
                    return true;
                } else if ((*this)[index] == ALP_Q && this->size() > 39) {
                    nType = TX_NEW_TOKEN;
                    fIsOwner = false;
                    return true;
                } else if ((*this)[index] == ALP_O) {
                    nType = TX_NEW_TOKEN;
                    fIsOwner = true;
                    return true;
                } else if ((*this)[index] == ALP_A) {
                    nType = TX_REISSUE_TOKEN;
                    return true;
                }
            }
        }
    }
    return false;
}


bool CScript::IsNewToken() const
{

    int nType = 0;
    bool fIsOwner = false;
    if (IsTokenScript(nType, fIsOwner))
        return !fIsOwner && nType == TX_NEW_TOKEN;

    return false;
}

bool CScript::IsOwnerToken() const
{
    int nType = 0;
    bool fIsOwner = false;
    if (IsTokenScript(nType, fIsOwner))
        return fIsOwner && nType == TX_NEW_TOKEN;

    return false;
}

bool CScript::IsReissueToken() const
{
    int nType = 0;
    bool fIsOwner = false;
    if (IsTokenScript(nType, fIsOwner))
        return nType == TX_REISSUE_TOKEN;

    return false;
}

bool CScript::IsTransferToken() const
{
    int nType = 0;
    bool fIsOwner = false;
    if (IsTokenScript(nType, fIsOwner))
        return nType == TX_TRANSFER_TOKEN;

    return false;
}
/** TOKENS END */

bool CScript::IsPayToWitnessScriptHash() const
{
    // Extra-fast test for pay-to-witness-script-hash CScripts:
    return (this->size() == 34 &&
            (*this)[0] == OP_0 &&
            (*this)[1] == 0x20);
}

// A witness program is any valid CScript that consists of a 1-byte push opcode
// followed by a data push between 2 and 40 bytes.
bool CScript::IsWitnessProgram(int& version, std::vector<unsigned char>& program) const
{
    if (this->size() < 4 || this->size() > 42) {
        return false;
    }
    if ((*this)[0] != OP_0 && ((*this)[0] < OP_1 || (*this)[0] > OP_16)) {
        return false;
    }
    if ((size_t)((*this)[1] + 2) == this->size()) {
        version = DecodeOP_N((opcodetype)(*this)[0]);
        program = std::vector<unsigned char>(this->begin() + 2, this->end());
        return true;
    }
    return false;
}
bool CScript::IsPayToPublicKey() const
{
    // Test for pay-to-pubkey CScript with both
    // compressed or uncompressed pubkey
    if (this->size() == 35) {
        return ((*this)[1] == 0x02 || (*this)[1] == 0x03) &&
                (*this)[34] == OP_CHECKSIG;
    }
    if (this->size() == 67) {
        return (*this)[1] == 0x04 &&
                (*this)[66] == OP_CHECKSIG;

    }
    return false;
}

bool CScript::IsPushOnly(const_iterator pc) const
{
    while (pc < end())
    {
        opcodetype opcode;
        if (!GetOp(pc, opcode))
            return false;
        // Note that IsPushOnly() *does* consider OP_RESERVED to be a
        // push-type opcode, however execution of OP_RESERVED fails, so
        // it's not relevant to P2SH/BIP62 as the scriptSig would fail prior to
        // the P2SH special validation code being executed.
        if (opcode > OP_16)
            return false;
    }
    return true;
}

bool CScript::IsPushOnly() const
{
    return this->IsPushOnly(begin());
}

std::string CScriptWitness::ToString() const
{
    std::string ret = "CScriptWitness(";
    for (unsigned int i = 0; i < stack.size(); i++) {
        if (i) {
            ret += ", ";
        }
        ret += HexStr(stack[i]);
    }
    return ret + ")";
}

bool CScript::HasValidOps() const
{
    CScript::const_iterator it = begin();
    while (it < end()) {
        opcodetype opcode;
        std::vector<unsigned char> item;
        if (!GetOp(it, opcode, item) || opcode > MAX_OPCODE || item.size() > MAX_SCRIPT_ELEMENT_SIZE) {
            return false;
        }
    }
    return true;
}

bool CScript::IsUnspendable() const
{
    CAmount nAmount;
    return (size() > 0 && *begin() == OP_RETURN) || (size() > MAX_SCRIPT_SIZE) || (GetTokenAmountFromScript(*this, nAmount) && nAmount == 0);
}

//!--------------------------------------------------------------------------------------------------------------------------!//
//! These are needed because script.h and script.cpp do not have access to token.h and token.cpp functions. This is
//! because the make file compiles them at different times. The script files are compiled with other
//! consensus files, and token files are compiled with core files.

//! Used to check if an token script contains zero tokens. Is so, it should be unspendable
bool GetTokenAmountFromScript(const CScript& script, CAmount& nAmount)
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
        if (AmountFromNewTokenScript(script, nAmount)) {
            return true;
        }
    } else if (type == TX_TRANSFER_TOKEN) {
        if (AmountFromTransferScript(script, nAmount)) {
            return true;
        }
    } else if (type == TX_NEW_TOKEN && fIsOwner) {
            nAmount = OWNER_TOKEN_AMOUNT;
            return true;
    } else if (type == TX_REISSUE_TOKEN) {
        if (AmountFromReissueScript(script, nAmount)) {
            return true;
        }
    }

    return false;
}

bool ScriptNewToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_NEW_TOKEN && !fIsOwner;
    }

    return false;
}

bool ScriptTransferToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_TRANSFER_TOKEN;
    }

    return false;
}

bool ScriptReissueToken(const CScript& scriptPubKey, int& nStartingIndex)
{
    int nType = 0;
    bool fIsOwner =false;
    if (scriptPubKey.IsTokenScript(nType, fIsOwner, nStartingIndex)) {
        return nType == TX_REISSUE_TOKEN;
    }

    return false;
}


bool AmountFromNewTokenScript(const CScript& scriptPubKey, CAmount& nAmount)
{
    int nStartingIndex = 0;
    if (!ScriptNewToken(scriptPubKey, nStartingIndex))
        return false;

    std::vector<unsigned char> vchNewToken;
    vchNewToken.insert(vchNewToken.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssToken(vchNewToken, SER_NETWORK, PROTOCOL_VERSION);

    CNewToken tokenNew;
    try {
        ssToken >> tokenNew;
    } catch(std::exception& e) {
        std::cout << "Failed to get the token from the stream: " << e.what() << std::endl;
        return false;
    }

    nAmount = tokenNew.nAmount;
    return true;
}

bool AmountFromTransferScript(const CScript& scriptPubKey, CAmount& nAmount)
{
    int nStartingIndex = 0;
    if (!ScriptTransferToken(scriptPubKey, nStartingIndex))
        return false;

    std::vector<unsigned char> vchToken;
    vchToken.insert(vchToken.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssToken(vchToken, SER_NETWORK, PROTOCOL_VERSION);

    CTokenTransfer token;
    try {
        ssToken >> token;
    } catch(std::exception& e) {
        std::cout << "Failed to get the token from the stream: " << e.what() << std::endl;
        return false;
    }

    nAmount = token.nAmount;
    return true;
}

bool AmountFromReissueScript(const CScript& scriptPubKey, CAmount& nAmount)
{
    int nStartingIndex = 0;
    if (!ScriptReissueToken(scriptPubKey, nStartingIndex))
        return false;

    std::vector<unsigned char> vchNewToken;
    vchNewToken.insert(vchNewToken.end(), scriptPubKey.begin() + nStartingIndex, scriptPubKey.end());
    CDataStream ssToken(vchNewToken, SER_NETWORK, PROTOCOL_VERSION);

    CReissueToken token;
    try {
        ssToken >> token;
    } catch(std::exception& e) {
        std::cout << "Failed to get the token from the stream: " << e.what() << std::endl;
        return false;
    }

    nAmount = token.nAmount;
    return true;
}
//!--------------------------------------------------------------------------------------------------------------------------!//


