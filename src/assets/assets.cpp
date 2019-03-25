// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// #include <assets/assetdb.h>
#include <assets/assets.h>
#include <script.h>
#include <base58.h>
#include <core.h>
#include <util.h>
#include <main.h>

#include <cmath>
#include <regex>
#include <string>

#define RVN_R 114
#define RVN_V 118
#define RVN_N 110
#define RVN_Q 113

static const std::regex ASSET_NAME_REGEX("[A-Za-z_]{3,}");

// Does static checking (length, characters, etc.)
bool IsAssetNameValid(const std::string& name)
{
	return std::regex_match(name, ASSET_NAME_REGEX);
}

bool IsAssetNameSizeValid(const std::string& name)
{
	return name.size() >= 3 && name.size() <= 31;
}

// 1, 10, 100 ... COIN
// (0.00000001, 0.0000001, ... 1)
bool IsAssetUnitsValid(const int64_t& units)
{
	for (int i = 1; i <= COIN; i *= 10) {
		if (units == i) return true;
	}
	return false;
}

bool CheckIssueBurnTx(const CTxOut& txOut)
{
	// Check the first transaction is the 500 Burn amount to the burn address
	if (txOut.nValue != 500 * COIN) { // Burn amount
		return false;
	}

	// Extract the destination
	CTxDestination destination;
	if (!ExtractDestination(txOut.scriptPubKey, destination)) {
		return false;
	}

	// Check destination address is the burn address
	if (CBitcoinAddress(destination).ToString() != "AehA882uo1S1yuowSyCRg5JNgF8pkjEbmg") { // Burn address
		return false;
	}

	return true;
}

bool IsScriptNewAsset(const CScript& scriptPubKey)
{
    if (scriptPubKey.size() > 39) {
        if (scriptPubKey[25] == OP_RETURN && scriptPubKey[27] == RVN_R && scriptPubKey[28] == RVN_V && scriptPubKey[29] == RVN_N && scriptPubKey[30] == RVN_Q) {
            return true;
        }
    }

    return false;
}

bool CheckIssueDataTx(const CTxOut& txOut)
{
    // Verify 'rvnq' is in the transaction
    CScript scriptPubKey = txOut.scriptPubKey;

    return IsScriptNewAsset(scriptPubKey);
}

bool CNewAsset::IsNull() const
{
    return strName == "";
}

bool CNewAsset::IsValid(std::string& strError, bool fCheckMempool)
{
    strError = "";

    // Check our current passets to see if the asset has been created yet
    if (passets->setAssets.count(*this))
        strError = std::string("Invalid parameter: asset_name '") + strName + std::string("' has already been used");

    // if (fCheckMempool) {
    //     for (const CTxMemPoolEntry &entry : mempool.mapTx) {
    //         CTransaction tx = entry.GetTx();
    //         if (tx.IsNewAsset()) {
    //             CNewAsset asset;
    //             std::string address;
    //             AssetFromTransaction(tx, asset, address);
    //             if (asset.strName == strName) {
    //                 strError = "Asset with this name is already in the mempool";
    //                 return false;
    //             }
    //         }
    //     }
    // }

    if (!IsAssetNameValid(std::string(strName)))
        strError = "Invalid parameter: asset_name must be only consist of valid characters. See help for more details.";

    if (!IsAssetNameSizeValid(strName))
        strError  = "Invalid parameter: asset_name must have a size between 3 to 31";

    if (nAmount <= 0)
        strError  = "Invalid parameter: asset amount can't be equal to or less than zero.";

    if (nNameLength < 1 || nNameLength > 9)
        strError  = "Invalid parameter: name_length must be between 1-9";

    if (units < 0 || units > 8)
        strError  = "Invalid parameter: units must be between 0-8.";

    if (nReissuable != 0 && nReissuable != 1)
        strError  = "Invalid parameter: reissuable must be 0 or 1";

    return strError == "";
}
