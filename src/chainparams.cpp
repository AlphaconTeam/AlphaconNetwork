// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Copyright (c) 2019 Alphacon developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "assert.h"

#include "chainparams.h"
#include "main.h"
#include "util.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

// Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7 * 24 * 60 * 60;
    for (unsigned int i = 0; i < count; i++) {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

static CBlock CreateGenesisBlock(const char* pszTimestamp, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion)
{
    CBlock genesis;
    std::vector<CTxIn> vin;
    vin.resize(1);
    vin[0].scriptSig = CScript() << 0 << CBigNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    std::vector<CTxOut> vout;
    vout.resize(1);
    vout[0].SetEmpty();
    CTransaction txNew(1, nTime, vin, vout, 0);
    genesis.vtx.push_back(txNew);
    genesis.hashPrevBlock = 0;
    genesis.hashMerkleRoot = genesis.BuildMerkleTree();
    genesis.nVersion = nVersion;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;

    return genesis;
}

static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion)
{
    const char* pszTimestamp = "Study: Sleep Deprivation May Damage Your DNA | Jan 29, 2019 Sci News";
    return CreateGenesisBlock(pszTimestamp, nTime, nNonce, nBits, nVersion);
}

// Main network params
class CMainParams : public CChainParams {
public:
    CMainParams() {
        pchMessageStart[0] = 0x41;
        pchMessageStart[1] = 0x4c;
        pchMessageStart[2] = 0x50;
        pchMessageStart[3] = 0x01;
        vAlertPubKey = ParseHex("04e7ed5e7037bb0938fc60b9164d9784d82ef56107f39c50095dfb3af06388960e6f2c6ec611fe82e7153cd0df0e65ed1a8d472a840180a7f85519e2eab3eddf0d");
        nDefaultPort = 19427;
        nRPCPort = 19428;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 20);

        blockRewardALP = 25000000000 * COIN;
        blockRewardHalvingsWindow = 262980; // Once per 6 months
        blockRewardHalvings = 2; // Only 2 times
        blockReward = 120 * COIN;

        rewardReceiverALP = "ALAHHWQvbzWjCj2UZrCzT7TMn6JyN8ogQm";
        rewardHeighALP = 1;
        
        genesis = CreateGenesisBlock(1548853998, 2004344, bnProofOfWorkLimit.GetCompact(), 1);
        hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256("0x00000bd194e16e8dc4bb9d3b6684c7757b203b3eec769e14e1492796736f304d"));
        assert(genesis.hashMerkleRoot == uint256("0xd40bfd444aa3049d8aaf3a212f4653ea81f5ad44b7d2fb94d3fc56b133b641f2"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 23);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 48);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1, 36);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        nLastPOWBlock = 1440;
    }

    virtual const CBlock& GenesisBlock() const { return genesis; }
    virtual Network NetworkID() const { return CChainParams::MAIN; }

    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};
static CMainParams mainParams;

// Test network
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        pchMessageStart[0] = 0x2d;
        pchMessageStart[1] = 0x34;
        pchMessageStart[2] = 0x30;
        pchMessageStart[3] = 0xe4;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 16);
        vAlertPubKey = ParseHex("040048058629c895fcdd63eddcd46a13e725714f0c9ef53f162336422b97ba6a2319acc0c5537e9b1cb1c91fa72243ae61b2d9610f741de92dd546cca56b46f76b");
        nDefaultPort = 29427;
        nRPCPort = 29428;
        strDataDir = "testnet";

        genesis = CreateGenesisBlock(1547942400, 214286, bnProofOfWorkLimit.GetCompact(), 1);
        hashGenesisBlock = genesis.GetHash();

        assert(hashGenesisBlock == uint256("0x0000c9f2bf15e9c3c286a24f5f1e5e1c868f77e8b91e6ea4caf7078a88c3cb34"));
        assert(genesis.hashMerkleRoot == uint256("0xe342a42f9f219923b128bc14f1e513ce2a7bf0dd4666cc11e877464fc2cc3ace"));

        vFixedSeeds.clear();
        vSeeds.clear();

        blockRewardALP = 25000000000 * COIN;
        blockRewardHalvingsWindow = 262980; // Once per 6 months
        blockRewardHalvings = 2; // Only 2 times
        blockReward = 120 * COIN;

        rewardReceiverALP = "caKbGCWqcDitmWzCGQ9W3Gwdc2vvQWbBov";
        rewardHeighALP = 1;

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 88);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 115);
        base58Prefixes[SECRET_KEY]     = std::vector<unsigned char>(1, 41);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        nLastPOWBlock = 1000;
    }
    virtual Network NetworkID() const { return CChainParams::TESTNET; }
};
static CTestNetParams testNetParams;

// Regression test params
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        pchMessageStart[0] = 0xaa;
        pchMessageStart[1] = 0xaf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        bnProofOfWorkLimit = CBigNum(~uint256(0) >> 1);
        genesis.nTime = 1000000000;
        genesis.nBits  = bnProofOfWorkLimit.GetCompact();
        genesis.nNonce = 13;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 39427;
        strDataDir = "regtest";

        blockReward = 120 * COIN;
        blockRewardALP = 25000000000 * COIN;
        blockRewardHalvingsWindow = 120;
        blockRewardHalvings = 2;

        rewardReceiverALP = "";
        rewardHeighALP = 1;

        assert(hashGenesisBlock == uint256("0x3922ac35c5b702b0be23f911c2921ac5c8b9c97243087f8d8aaf5fa29e30e602"));

        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const { return false; }
    virtual Network NetworkID() const { return CChainParams::REGTEST; }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = &mainParams;

const CChainParams &Params() {
    return *pCurrentParams;
}

void SelectParams(CChainParams::Network network) {
    switch (network) {
        case CChainParams::MAIN:
            pCurrentParams = &mainParams;
            break;
        case CChainParams::TESTNET:
            pCurrentParams = &testNetParams;
            break;
        case CChainParams::REGTEST:
            pCurrentParams = &regTestParams;
            break;
        default:
            assert(false && "Unimplemented network");
            return;
    }
}

bool SelectParamsFromCommandLine() {
    bool fRegTest = GetBoolArg("-regtest", false);
    bool fTestNet = GetBoolArg("-testnet", false);

    if (fTestNet && fRegTest) {
        return false;
    }

    if (fRegTest) {
        SelectParams(CChainParams::REGTEST);
    } else if (fTestNet) {
        SelectParams(CChainParams::TESTNET);
    } else {
        SelectParams(CChainParams::MAIN);
    }
    return true;
}
