// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

using namespace std;

static CBlock CreateGenesisBlock(const char* pszTimestamp, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
	CMutableTransaction txNew;
	txNew.nTime = nTime;
	txNew.vin.resize(1);
	txNew.vout.resize(1);
	txNew.vin[0].scriptSig = CScript() << 0 << CScriptNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
	txNew.vout[0].SetEmpty();

	CBlock genesis;
	genesis.vtx.push_back(txNew);
	genesis.hashPrevBlock.SetNull();
	genesis.nVersion = nVersion;
	genesis.nTime    = nTime;
	genesis.nBits    = nBits;
	genesis.nNonce   = nNonce;
	genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
	return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
	const char* pszTimestamp = "Study: Sleep Deprivation May Damage Your DNA | Jan 29, 2019 Sci News";
	return CreateGenesisBlock(pszTimestamp, nTime, nNonce, nBits, nVersion, genesisReward);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */

class CMainParams : public CChainParams {
public:
	CMainParams() {
		strNetworkID = "main";
		consensus.nMaxReorganizationDepth = 500;
		consensus.nMajorityEnforceBlockUpgrade = 750;
		consensus.nMajorityRejectBlockOutdated = 950;
		consensus.nMajorityWindow = 1000;
		consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
		consensus.posLimit = uint256S("000000000000ffffffffffffffffffffffffffffffffffffffffffffffffffff");
		consensus.nTargetTimespan = 16 * 60; // 16 mins
		consensus.nTargetSpacing = 64;
		consensus.BIP34Height = -1;
		consensus.BIP34Hash = uint256();
		consensus.fPowAllowMinDifficultyBlocks = false;
		consensus.fPowNoRetargeting = false;
		consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
		consensus.nMinerConfirmationWindow = 2016; // nTargetTimespan / nTargetSpacing
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

		consensus.nLastPOWBlock = 1440;
		consensus.nStakeTimestampMask = 0xf; // 15
		consensus.nCoinbaseMaturity = 100;
		consensus.nStakeMinConfirmations = 450;
		consensus.nStakeMinAge = 1 * 60 * 60; // 1 hour
		consensus.nGenesisTimestamp = 1548853998;
		consensus.nBlockRewardHalvingsWindow = 262980;
		consensus.nBlockRewardHalvings = 2;
		consensus.nBlockReward = 120 * COIN;
		consensus.nBlockRewardALP = 25000000000 * COIN;;
		consensus.nRewardHeighALP = 1;

		nIssueAssetBurnAmount = 500 * COIN;
        strIssueAssetBurnAddress = "AGcWpX7M5Hueqc3KgWN413MpR1Gi82narG";

		/**
		 * The message start string is designed to be unlikely to occur in normal data.
		 * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
		 * a large 32-bit integer with any alignment.
		 */
		pchMessageStart[0] = 0x41;
		pchMessageStart[1] = 0x4c;
		pchMessageStart[2] = 0x50;
		pchMessageStart[3] = 0x01;
		vAlertPubKey = ParseHex("04e7ed5e7037bb0938fc60b9164d9784d82ef56107f39c50095dfb3af06388960e6f2c6ec611fe82e7153cd0df0e65ed1a8d472a840180a7f85519e2eab3eddf0d");
		nDefaultPort = 19427;
		nMaxTipAge = 24 * 60 * 60;
		nPruneAfterHeight = 100000;

		const char* pszTimestamp = "Study: Sleep Deprivation May Damage Your DNA | Jan 29, 2019 Sci News";
        CMutableTransaction txNew;
        txNew.nTime = consensus.nGenesisTimestamp;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 0 << CScriptNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].SetEmpty();
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock.SetNull();
        genesis.nVersion = 1;
        genesis.nTime    = consensus.nGenesisTimestamp;
        genesis.nBits    = 0x1e0fffff;
        genesis.nNonce   = 2004344;
        genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
		consensus.hashGenesisBlock = genesis.GetHash();

		assert(consensus.hashGenesisBlock == uint256S("0x00000bd194e16e8dc4bb9d3b6684c7757b203b3eec769e14e1492796736f304d"));
		assert(genesis.hashMerkleRoot == uint256S("0xd40bfd444aa3049d8aaf3a212f4653ea81f5ad44b7d2fb94d3fc56b133b641f2"));

		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,23);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,48);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1,36);
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();
		cashaddrPrefix = "alphacon";

		vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

		fMiningRequiresPeers = true;
		fDefaultConsistencyChecks = false;
		fRequireStandard = true;
		fMineBlocksOnDemand = false;
		fTestnetToBeDeprecatedFieldRPC = false;

		checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            ( 0, uint256S("0x00000bd194e16e8dc4bb9d3b6684c7757b203b3eec769e14e1492796736f304d")),
            0,
            0,
            0
        };
	}
};
static CMainParams mainParams;

/**
 * Testnet
 */
class CTestNetParams : public CChainParams {
public:
	CTestNetParams() {
		strNetworkID = "test";
		consensus.nMaxReorganizationDepth = 50;
		consensus.nMajorityEnforceBlockUpgrade = 51;
		consensus.nMajorityRejectBlockOutdated = 75;
		consensus.nMajorityWindow = 100;
		consensus.powLimit = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
		consensus.posLimit = uint256S("000000000000ffffffffffffffffffffffffffffffffffffffffffffffffffff");
		consensus.nTargetTimespan = 16 * 60; // 16 mins
		consensus.nTargetSpacing = 64;
		consensus.BIP34Height = -1;
		consensus.BIP34Hash = uint256();
		consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowNoRetargeting = false;
		consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
		consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

		consensus.nLastPOWBlock = 0x7fffffff;
		consensus.nStakeTimestampMask = 0xf; // 15
		consensus.nCoinbaseMaturity = 50;
		consensus.nStakeMinConfirmations = 50;
		consensus.nStakeMinAge = 1 * 60 * 60; // 1 hour

		pchMessageStart[0] = 0xcd;
		pchMessageStart[1] = 0xf2;
		pchMessageStart[2] = 0xc0;
		pchMessageStart[3] = 0xef;
		vAlertPubKey = ParseHex("0471dc165db490094d35cde15b1f5d755fa6ad6f2b5ed0f340e3f17f57389c3c2af113a8cbcc885bde73305a553b5640c83021128008ddf882e856336269080496");
		nDefaultPort = 25714;
		nMaxTipAge = 0x7fffffff;
		nPruneAfterHeight = 1000;

		genesis = CreateGenesisBlock(1393221600, 216178, 0x1f00ffff, 1, 0);
		consensus.hashGenesisBlock = genesis.GetHash();

		assert(consensus.hashGenesisBlock == uint256S("0x8185431cf94b77950bba7d6ce098bbe3d8bc1ecba91f84cfac3bb05489077244"));
		assert(genesis.hashMerkleRoot == uint256S("0xd1647d542ddc995f38c86b6e312d2beb9b7974726c7b1ac8e1b8a4dfd9c0771a"));

		vFixedSeeds.clear();
		vSeeds.clear();

		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,111);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,196);
		base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();
		cashaddrPrefix = "blktest";

		vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

		fMiningRequiresPeers = true;
		fDefaultConsistencyChecks = false;
		fRequireStandard = false;
		fMineBlocksOnDemand = false;
		fTestnetToBeDeprecatedFieldRPC = true;

		checkpointData = (CCheckpointData) {
			boost::assign::map_list_of
			( 0, uint256S("0x0000724595fb3b9609d441cbfb9577615c292abf07d996d3edabc48de843642d")),
			0,
			0,
			0
		};

	}
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
	CRegTestParams() {
		strNetworkID = "regtest";
		consensus.nMaxReorganizationDepth = 500;
		consensus.nMajorityEnforceBlockUpgrade = 750;
		consensus.nMajorityRejectBlockOutdated = 950;
		consensus.nMajorityWindow = 1000;
		consensus.powLimit = uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
		consensus.posLimit = uint256S("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
		consensus.nTargetTimespan = 16 * 60; // 16 mins
		consensus.nTargetSpacing = 64;
		consensus.BIP34Height = -1; // BIP34 has not necessarily activated on regtest
		consensus.BIP34Hash = uint256();
		consensus.fPowAllowMinDifficultyBlocks = true;
		consensus.fPowNoRetargeting = true;
		consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
		consensus.nMinerConfirmationWindow = 2016; // nTargetTimespan / nTargetSpacing
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
		consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008
		
		consensus.nLastPOWBlock = 10000;
		consensus.nStakeTimestampMask = 0xf;

		/**
		 * The message start string is designed to be unlikely to occur in normal data.
		 * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
		 * a large 32-bit integer with any alignment.
		 */
		pchMessageStart[0] = 0x70;
		pchMessageStart[1] = 0x35;
		pchMessageStart[2] = 0x22;
		pchMessageStart[3] = 0x06;
		vAlertPubKey = ParseHex("042508124261e3c969d9b4988349c41a329c6979e446facffc3227e16e5420c366e7d917e8ef80e70a27b90582272c93b6d0f16b0c728b970f73478167729cbbea");
		nDefaultPort = 25714;
		nMaxTipAge = 0x7fffffff;
		nPruneAfterHeight = 100000;

		/**
				 * Build the genesis block. Note that the output of its generation
				 * transaction cannot be spent since it did not originally exist in the
				 * database.
				 *
				 * CBlock(hash=000001faef25dec4fbcf906e6242621df2c183bf232f263d0ba5b101911e4563, ver=1, hashPrevBlock=0000000000000000000000000000000000000000000000000000000000000000, hashMerkleRoot=12630d16a97f24b287c8c2594dda5fb98c9e6c70fc61d44191931ea2aa08dc90, nTime=1393221600, nBits=1e0fffff, nNonce=164482, vtx=1, vchBlockSig=)
				 *   Coinbase(hash=12630d16a9, ver=1, nTime=1393221600, vin.size=1, vout.size=1, nLockTime=0)
				 *     CTxIn(COutPoint(0000000000, 4294967295), coinbase 00012a24323020466562203230313420426974636f696e2041544d7320636f6d6520746f20555341)
				 *     CTxOut(empty)
				 *   vMerkleTree: 12630d16a9
				 */
		const char* pszTimestamp = "20 Feb 2014 Bitcoin ATMs come to USA";
		CMutableTransaction txNew;
		txNew.nTime = 1393221600;
		txNew.vin.resize(1);
		txNew.vout.resize(1);
		txNew.vin[0].scriptSig = CScript() << 0 << CScriptNum(42) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
		txNew.vout[0].SetEmpty();
		genesis.vtx.push_back(txNew);
		genesis.hashPrevBlock.SetNull();
		genesis.nVersion = 1;
		genesis.nTime    = 1393221600;
		genesis.nBits    = 0x1e0fffff;
		genesis.nNonce = 164482;
		genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
		consensus.hashGenesisBlock = genesis.GetHash();

		assert(consensus.hashGenesisBlock == uint256S("0x3c70e34e8d07e87402fb1cc5ddf29b7ff79dc2e90509f13f51b2a1ca6c2bd836"));
		assert(genesis.hashMerkleRoot == uint256S("0x12630d16a97f24b287c8c2594dda5fb98c9e6c70fc61d44191931ea2aa08dc90"));

		vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
		vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

		base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,25);
		base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,85);
		base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1,153);
		base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
		base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();
		cashaddrPrefix = "blkreg";

		fMiningRequiresPeers = false;
		fDefaultConsistencyChecks = true;
		fRequireStandard = false;
		fMineBlocksOnDemand = true;
		fTestnetToBeDeprecatedFieldRPC = false;

		checkpointData = (CCheckpointData) {
			boost::assign::map_list_of
			( 0, uint256S("0x000001faef25dec4fbcf906e6242621df2c183bf232f263d0ba5b101911e4563")),
			0,
			0,
			0
		};

	}
};
static CRegTestParams regTestParams;



static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
	assert(pCurrentParams);
	return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
	if (chain == CBaseChainParams::MAIN)
			return mainParams;
	else if (chain == CBaseChainParams::TESTNET)
			return testNetParams;
	else if (chain == CBaseChainParams::REGTEST)
			return regTestParams;
	else
		throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
	SelectBaseParams(network);
	pCurrentParams = &Params(network);
}

