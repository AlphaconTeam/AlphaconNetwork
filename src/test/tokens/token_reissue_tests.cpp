// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <tokens/tokens.h>

#include <test/test_alphacon.h>

#include <boost/test/unit_test.hpp>

#include <amount.h>
#include <script/standard.h>
#include <base58.h>
#include <consensus/validation.h>
#include <consensus/tx_verify.h>
#include <validation.h>
BOOST_FIXTURE_TEST_SUITE(token_reissue_tests, BasicTestingSetup)


    BOOST_AUTO_TEST_CASE(reissue_cache_test)
    {
        BOOST_TEST_MESSAGE("Running Reissue Cache Test");

        SelectParams(CBaseChainParams::MAIN);

        fTokenIndex = true; // We only cache if fTokenIndex is true
        ptokens = new CTokensCache();
        // Create tokens cache
        CTokensCache cache;

        CNewToken token1("ALPTOKEN", CAmount(100 * COIN), 8, 1, 0, "");

        // Add an token to a valid ALP address
        uint256 hash = uint256();
        BOOST_CHECK_MESSAGE(cache.AddNewToken(token1, Params().GlobalBurnAddress(), 0, hash), "Failed to add new token");

        // Create a reissuance of the token
        CReissueToken reissue1("ALPTOKEN", CAmount(1 * COIN), 8, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));
        COutPoint out(uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A4"), 1);

        // Add an reissuance of the token to the cache
        BOOST_CHECK_MESSAGE(cache.AddReissueToken(reissue1, Params().GlobalBurnAddress(), out), "Failed to add reissue");

        // Check to see if the reissue changed the cache data correctly
        BOOST_CHECK_MESSAGE(cache.mapReissuedTokenData.count("ALPTOKEN"), "Map Reissued Token should contain the token \"ALPTOKEN\"");
        BOOST_CHECK_MESSAGE(cache.mapTokensAddressAmount.at(make_pair("ALPTOKEN", Params().GlobalBurnAddress())) == CAmount(101 * COIN), "Reissued amount wasn't added to the previous total");

        // Get the new token data from the cache
        CNewToken token2;
        BOOST_CHECK_MESSAGE(cache.GetTokenMetaDataIfExists("ALPTOKEN", token2), "Failed to get the token2");

        // Chech the token metadata
        BOOST_CHECK_MESSAGE(token2.nReissuable == 1, "Token2: Reissuable isn't 1");
        BOOST_CHECK_MESSAGE(token2.nAmount == CAmount(101 * COIN), "Token2: Amount isn't 101");
        BOOST_CHECK_MESSAGE(token2.strName == "ALPTOKEN", "Token2: Token name is wrong");
        BOOST_CHECK_MESSAGE(token2.units == 8, "Token2: Units is wrong");
        BOOST_CHECK_MESSAGE(EncodeIPFS(token2.strIPFSHash) == "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo", "Token2: IPFS hash is wrong");

        // Remove the reissue from the cache
        std::vector<std::pair<std::string, CBlockTokenUndo> > undoBlockData;
        undoBlockData.emplace_back(std::make_pair("ALPTOKEN", CBlockTokenUndo{true, false, "", 0}));
        BOOST_CHECK_MESSAGE(cache.RemoveReissueToken(reissue1, Params().GlobalBurnAddress(), out, undoBlockData), "Failed to remove reissue");

        // Get the token data from the cache now that the reissuance was removed
        CNewToken token3;
        BOOST_CHECK_MESSAGE(cache.GetTokenMetaDataIfExists("ALPTOKEN", token3), "Failed to get the token3");

        // Chech the token3 metadata and make sure all the changed from the reissue were removed
        BOOST_CHECK_MESSAGE(token3.nReissuable == 1, "Token3: Reissuable isn't 1");
        BOOST_CHECK_MESSAGE(token3.nAmount == CAmount(100 * COIN), "Token3: Amount isn't 100");
        BOOST_CHECK_MESSAGE(token3.strName == "ALPTOKEN", "Token3: Token name is wrong");
        BOOST_CHECK_MESSAGE(token3.units == 8, "Token3: Units is wrong");
        BOOST_CHECK_MESSAGE(token3.strIPFSHash == "", "Token3: IPFS hash is wrong");

        // Check to see if the reissue removal updated the cache correctly
        BOOST_CHECK_MESSAGE(cache.mapReissuedTokenData.count("ALPTOKEN"), "Map of reissued data was removed, even though changes were made and not databased yet");
        BOOST_CHECK_MESSAGE(cache.mapTokensAddressAmount.at(make_pair("ALPTOKEN", Params().GlobalBurnAddress())) == CAmount(100 * COIN), "Tokens total wasn't undone when reissuance was");
    }


    BOOST_AUTO_TEST_CASE(reissue_isvalid_test)
    {
        BOOST_TEST_MESSAGE("Running Reissue IsValid Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create tokens cache
        CTokensCache cache;

        CNewToken token1("ALPTOKEN", CAmount(100 * COIN), 8, 1, 0, "");

        // Add an token to a valid ALP address
        BOOST_CHECK_MESSAGE(cache.AddNewToken(token1, Params().GlobalBurnAddress(), 0, uint256()), "Failed to add new token");

        // Create a reissuance of the token that is valid
        CReissueToken reissue1("ALPTOKEN", CAmount(1 * COIN), 8, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        std::string error;
        BOOST_CHECK_MESSAGE(reissue1.IsValid(error, cache), "Reissue should of been valid");

        // Create a reissuance of the token that is not valid
        CReissueToken reissue2("NOTEXIST", CAmount(1 * COIN), 8, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(!reissue2.IsValid(error, cache), "Reissue shouldn't of been valid");

        // Create a reissuance of the token that is not valid (unit is smaller than current token)
        CReissueToken reissue3("ALPTOKEN", CAmount(1 * COIN), 7, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(!reissue3.IsValid(error, cache), "Reissue shouldn't of been valid because of units");

        // Create a reissuance of the token that is not valid (unit is not changed)
        CReissueToken reissue4("ALPTOKEN", CAmount(1 * COIN), -1, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(reissue4.IsValid(error, cache), "Reissue4 wasn't valid");

        // Create a new token object with units of 0
        CNewToken token2("ALPTOKEN2", CAmount(100 * COIN), 0, 1, 0, "");

        // Add new token to a valid ALP address
        BOOST_CHECK_MESSAGE(cache.AddNewToken(token2, Params().GlobalBurnAddress(), 0, uint256()), "Failed to add new token");

        // Create a reissuance of the token that is valid unit go from 0 -> 1 and change the ipfs hash
        CReissueToken reissue5("ALPTOKEN2", CAmount(1 * COIN), 1, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(reissue5.IsValid(error, cache), "Reissue5 wasn't valid");

        // Create a reissuance of the token that is valid unit go from 1 -> 1 and change the ipfs hash
        CReissueToken reissue6("ALPTOKEN2", CAmount(1 * COIN), 1, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        BOOST_CHECK_MESSAGE(reissue6.IsValid(error, cache), "Reissue6 wasn't valid");
    }


BOOST_AUTO_TEST_SUITE_END()