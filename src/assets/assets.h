// Copyright (c) 2017 The Raven Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ALPHACON_ASSET_PROTOCOL_H
#define ALPHACON_ASSET_PROTOCOL_H

#include <serialize.h>

#include <string>
#include <set>
#include <map>

class CScript;
class CDataStream;
class CTransaction;
class COutPoint;
class CTxOut;
	
class CNewAsset {
public:
	int8_t nNameLength;
	std::string strName;
	int64_t nAmount;
	int8_t units;
	int8_t nReissuable;

	CNewAsset()
	{
		SetNull();
	}

	CNewAsset(const std::string& strName, const int64_t& nAmount, const int& nNameLength, const int& units, const int& nReissuable);

	void SetNull()
	{
		nNameLength = int8_t(1);
		strName= "";
		nAmount = 0;
		units = int8_t(1);
		nReissuable = int8_t(0);
	}

	bool IsNull() const;

	bool IsValid(std::string& strError, bool fCheckMempool = false);

	std::string ToString();

	IMPLEMENT_SERIALIZE
    (
       	READWRITE(nNameLength);
		READWRITE(strName);
		READWRITE(nAmount);
		READWRITE(units);
		READWRITE(nReissuable);
    )

	void ConstructTransaction(CScript& script) const;

};

class AssetComparator
{
public:
    bool operator()(const CNewAsset& s1, const CNewAsset& s2) const
    {
        return s1.strName < s2.strName;
    }
};

class CAssets {
public:
    std::set<CNewAsset, AssetComparator> setAssets;
    std::map<std::string, std::set<COutPoint> > mapMyUnspentAssets; // Asset Name -> COutPoint
    std::map<std::string, std::set<COutPoint> > mapMySpentAssets;
    std::map<std::string, std::set<std::string> > mapAssetsAddresses; // Asset Name -> set <Addresses>
    std::map<std::pair<std::string, std::string>, int64_t> mapAssetsAddressAmount; // pair < Asset Name , Address > -> Quantity of tokens in the address

    CAssets() {
        SetNull();
    }

    void SetNull() {
        setAssets.clear();
        mapMyUnspentAssets.clear();
        mapMySpentAssets.clear();
        mapAssetsAddresses.clear();
        mapAssetsAddressAmount.clear();
    }

    bool AddNewAsset(const CNewAsset& asset, const std::string& address);
    bool AddToMyUpspentOutPoints(const std::string& strName, const COutPoint& out);

    bool ContainsAsset(const CNewAsset& asset);
    bool RemoveAssetAndOutPoints(const CNewAsset& asset, const std::string& address);
};

bool IsAssetNameValid(const std::string& name);

bool IsAssetUnitsValid(const int64_t& units);

#endif //ALPHACON_ASSET_PROTOCOL_H
