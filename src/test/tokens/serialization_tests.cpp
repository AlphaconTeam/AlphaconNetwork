// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tokens/tokens.h>

#include <test/test_alphacon.h>

#include <boost/test/unit_test.hpp>

#include <amount.h>
#include <base58.h>
#include <chainparams.h>

BOOST_FIXTURE_TEST_SUITE(serialization_tests, BasicTestingSetup)

    BOOST_AUTO_TEST_CASE(issue_token_serialization_test)
    {
        BOOST_TEST_MESSAGE("Running Issue Token Serialization Test");

        SelectParams("test");

        // Create token
        CNewToken token("SERIALIZATION", 100000000, 0, 0, 1, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        // Create destination
        CTxDestination dest = DecodeDestination("mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp"); // Testnet Address

        BOOST_CHECK(IsValidDestination(dest));

        CScript scriptPubKey = GetScriptForDestination(dest);

        token.ConstructTransaction(scriptPubKey);

        CNewToken serializedToken;
        std::string address;
        BOOST_CHECK_MESSAGE(TokenFromScript(scriptPubKey, serializedToken, address), "Failed to get token from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken.strName == "SERIALIZATION", "Token names weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken.units == 0, "Units weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken.nReissuable == 0, "Reissuable wasn't equal");
        BOOST_CHECK_MESSAGE(serializedToken.nHasIPFS == 1, "HasIPFS wasn't equal");
        BOOST_CHECK_MESSAGE(EncodeIPFS(serializedToken.strIPFSHash) == "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo", "IPFSHash wasn't equal");

        // Bare token
        CNewToken token2("SERIALIZATION", 100000000);
        scriptPubKey = GetScriptForDestination(dest);
        token2.ConstructTransaction(scriptPubKey);
        CNewToken serializedToken2;
        BOOST_CHECK_MESSAGE(TokenFromScript(scriptPubKey, serializedToken2, address), "Failed to get token from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.strName == "SERIALIZATION", "Token names weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.units == 0, "Units weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.nReissuable == 1, "Reissuable wasn't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.nHasIPFS == 0, "HasIPFS wasn't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.strIPFSHash == "", "IPFSHash wasn't equal");
    }

    BOOST_AUTO_TEST_CASE(reissue_token_serialization_test)
    {
        BOOST_TEST_MESSAGE("Running Reissue Token Serialization Test");

        SelectParams("test");

        // Create token
        std::string name = "SERIALIZATION";
        CReissueToken reissue(name, 100000000, 0, 0, DecodeIPFS("QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo"));

        // Create destination
        CTxDestination dest = DecodeDestination("mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp"); // Testnet Address

        BOOST_CHECK(IsValidDestination(dest));

        CScript scriptPubKey = GetScriptForDestination(dest);

        reissue.ConstructTransaction(scriptPubKey);

        CReissueToken serializedToken;
        std::string address;
        BOOST_CHECK_MESSAGE(ReissueTokenFromScript(scriptPubKey, serializedToken, address), "Failed to get token from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken.strName == "SERIALIZATION", "Token names weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(EncodeIPFS(serializedToken.strIPFSHash) == "QmacSRmrkVmvJfbCpmU6pK72furJ8E8fbKHindrLxmYMQo", "IPFSHash wasn't equal");

        // Empty IPFS
        CReissueToken reissue2(name, 100000000, 0, 0, "");
        scriptPubKey = GetScriptForDestination(dest);
        reissue2.ConstructTransaction(scriptPubKey);
        CReissueToken serializedToken2;
        BOOST_CHECK_MESSAGE(ReissueTokenFromScript(scriptPubKey, serializedToken2, address), "Failed to get token from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.strName == "SERIALIZATION", "Token names weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.nAmount == 100000000, "Amount weren't equal");
        BOOST_CHECK_MESSAGE(serializedToken2.strIPFSHash == "", "IPFSHash wasn't equal");
    }

    BOOST_AUTO_TEST_CASE(owner_token_serialization_test)
    {
        BOOST_TEST_MESSAGE("Running Owner Token Serialization Test");

        SelectParams("test");

        // Create token
        std::string name = "SERIALIZATION";
        CNewToken token(name, 100000000);

        // Create destination
        CTxDestination dest = DecodeDestination("mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp"); // Testnet Address

        BOOST_CHECK(IsValidDestination(dest));

        CScript scriptPubKey = GetScriptForDestination(dest);

        token.ConstructOwnerTransaction(scriptPubKey);

        std::string strOwnerName;
        std::string address;
        std::stringstream ownerName;
        ownerName << name << OWNER_TAG;
        BOOST_CHECK_MESSAGE(OwnerTokenFromScript(scriptPubKey, strOwnerName, address), "Failed to get token from script");
        BOOST_CHECK_MESSAGE(address == "mfe7MqgYZgBuXzrT2QTFqZwBXwRDqagHTp", "Addresses weren't equal");
        BOOST_CHECK_MESSAGE(strOwnerName == ownerName.str(), "Token names weren't equal");
    }

BOOST_AUTO_TEST_SUITE_END()