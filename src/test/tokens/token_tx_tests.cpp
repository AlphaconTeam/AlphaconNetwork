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

BOOST_FIXTURE_TEST_SUITE(token_tx_tests, BasicTestingSetup)

    BOOST_AUTO_TEST_CASE(token_tx_valid_test)
    {
        BOOST_TEST_MESSAGE("Running Token TX Valid Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create the token scriptPubKey
        CTokenTransfer token("ALPHACON", 1000, 0);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0, 0, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);
        mutTx.vout.emplace_back(txOut);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 1000 Tokens
        // The outputs are assigning a destination to 1000 Tokens
        // This test should pass because all tokens are assigned a destination
        std::vector<std::pair<std::string, uint256>> vReissueTokens;
        BOOST_CHECK_MESSAGE(Consensus::CheckTxTokens(tx, state, coins, 100000, 100, vReissueTokens, true), "CheckTxTokens Failed");
    }

    BOOST_AUTO_TEST_CASE(token_tx_not_valid_test)
    {
        BOOST_TEST_MESSAGE("Running Token TX Not Valid Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create the token scriptPubKey
        CTokenTransfer token("ALPHACON", 1000, 0);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0, 0, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Create CTxOut that will only send 100 of the token
        // This should fail because 900 ALPHACON doesn't have a destination
        CTokenTransfer tokenTransfer("ALPHACON", 100, 0);
        CScript scriptLess = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        tokenTransfer.ConstructTransaction(scriptLess);

        CTxOut txOut2;
        txOut2.nValue = 0;
        txOut2.scriptPubKey = scriptLess;

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);
        mutTx.vout.emplace_back(txOut2);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs of this transaction are spending 1000 Tokens
        // The outputs are assigning a destination to only 100 Tokens
        // This should fail because 900 Tokens aren't being assigned a destination (Trying to burn 900 Tokens)
        std::vector<std::pair<std::string, uint256>> vReissueTokens;
        BOOST_CHECK_MESSAGE(!Consensus::CheckTxTokens(tx, state, coins, 100000, 100, vReissueTokens, true), "CheckTxTokens should of failed");
    }

    BOOST_AUTO_TEST_CASE(token_tx_valid_multiple_outs_test)
    {
        BOOST_TEST_MESSAGE("Running Token TX Valid Multiple Outs Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create the token scriptPubKey
        CTokenTransfer token("ALPHACON", 1000, 0);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0, 0, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Create CTxOut that will only send 100 of the token 10 times total = 1000
        for (int i = 0; i < 10; i++)
        {
            CTokenTransfer token2("ALPHACON", 100, 0);
            CScript scriptPubKey2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            token2.ConstructTransaction(scriptPubKey2);

            CTxOut txOut2;
            txOut2.nValue = 0;
            txOut2.scriptPubKey = scriptPubKey2;

            // Add the output into the transaction
            mutTx.vout.emplace_back(txOut2);
        }

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 1000 Tokens
        // The outputs are assigned 100 Tokens to 10 destinations (10 * 100) = 1000
        // This test should pass all tokens that are being spent are assigned to a destination
        std::vector<std::pair<std::string, uint256>> vReissueTokens;
        BOOST_CHECK_MESSAGE(Consensus::CheckTxTokens(tx, state, coins, 100000, 100, vReissueTokens, true), "CheckTxTokens failed");
    }

    BOOST_AUTO_TEST_CASE(token_tx_multiple_outs_invalid_test)
    {
        BOOST_TEST_MESSAGE("Running Token TX Multiple Outs Invalid Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create the token scriptPubKey
        CTokenTransfer token("ALPHACON", 1000, 0);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token.ConstructTransaction(scriptPubKey);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOut and add it to a coin
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");

        // Add the coin to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0, 0, 0), true);

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        // Create CTxOut that will only send 100 of the token 12 times, total = 1200
        for (int i = 0; i < 12; i++)
        {
            CTokenTransfer token2("ALPHACON", 100, 0);
            CScript scriptPubKey2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            token2.ConstructTransaction(scriptPubKey2);

            CTxOut txOut2;
            txOut2.nValue = 0;
            txOut2.scriptPubKey = scriptPubKey2;

            // Add the output into the transaction
            mutTx.vout.emplace_back(txOut2);
        }

        // Add the input, and an output into the transaction
        mutTx.vin.emplace_back(in);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 1000 Tokens
        // The outputs are assigning 100 Tokens to 12 destinations (12 * 100 = 1200)
        // This test should fail because the Outputs are greater than the inputs
        std::vector<std::pair<std::string, uint256>> vReissueTokens;
        BOOST_CHECK_MESSAGE(!Consensus::CheckTxTokens(tx, state, coins, 100000, 100, vReissueTokens, true), "CheckTxTokens passed when it should of failed");
    }

    BOOST_AUTO_TEST_CASE(token_tx_multiple_tokens_test)
    {
        BOOST_TEST_MESSAGE("Running Token TX Multiple Tokens Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create the token scriptPubKeys
        CTokenTransfer token("ALPHACON", 1000, 0);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token.ConstructTransaction(scriptPubKey);

        CTokenTransfer token2("ALPHACONTEST", 1000, 0);
        CScript scriptPubKey2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token2.ConstructTransaction(scriptPubKey2);

        CTokenTransfer token3("ALPHACONTESTTEST", 1000, 0);
        CScript scriptPubKey3 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token3.ConstructTransaction(scriptPubKey3);

        CCoinsView view;
        CCoinsViewCache coins(&view);

        // Create CTxOuts
        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        CTxOut txOut2;
        txOut2.nValue = 0;
        txOut2.scriptPubKey = scriptPubKey2;

        CTxOut txOut3;
        txOut3.nValue = 0;
        txOut3.scriptPubKey = scriptPubKey3;

        // Create a random hash
        uint256 hash = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A2");
        uint256 hash2 = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A3");
        uint256 hash3 = uint256S("BF50CB9A63BE0019171456252989A459A7D0A5F494735278290079D22AB704A4");

        // Add the coins to the cache
        COutPoint outpoint(hash, 1);
        coins.AddCoin(outpoint, Coin(txOut, 10, 0, 0, 0), true);

        COutPoint outpoint2(hash2, 1);
        coins.AddCoin(outpoint2, Coin(txOut2, 10, 0, 0, 0), true);

        COutPoint outpoint3(hash3, 1);
        coins.AddCoin(outpoint3, Coin(txOut3, 10, 0, 0, 0), true);

        Coin coinTemp;
        BOOST_CHECK_MESSAGE(coins.GetCoin(outpoint, coinTemp), "Failed to get coin 1");
        BOOST_CHECK_MESSAGE(coins.GetCoin(outpoint2, coinTemp), "Failed to get coin 2");
        BOOST_CHECK_MESSAGE(coins.GetCoin(outpoint3, coinTemp), "Failed to get coin 3");

        // Create transaction and input for the outpoint of the coin we just created
        CMutableTransaction mutTx;

        CTxIn in;
        in.prevout = outpoint;

        CTxIn in2;
        in2.prevout = outpoint2;

        CTxIn in3;
        in3.prevout = outpoint3;

        // Create CTxOut for each token that spends 100 tokens 10 time = 1000 token in total
        for (int i = 0; i < 10; i++)
        {
            // Add the first token
            CTokenTransfer outToken("ALPHACON", 100, 0);
            CScript outScript = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outToken.ConstructTransaction(outScript);

            CTxOut txOutNew;
            txOutNew.nValue = 0;
            txOutNew.scriptPubKey = outScript;

            mutTx.vout.emplace_back(txOutNew);

            // Add the second token
            CTokenTransfer outToken2("ALPHACONTEST", 100, 0);
            CScript outScript2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outToken2.ConstructTransaction(outScript2);

            CTxOut txOutNew2;
            txOutNew2.nValue = 0;
            txOutNew2.scriptPubKey = outScript2;

            mutTx.vout.emplace_back(txOutNew2);

            // Add the third token
            CTokenTransfer outToken3("ALPHACONTESTTEST", 100, 0);
            CScript outScript3 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outToken3.ConstructTransaction(outScript3);

            CTxOut txOutNew3;
            txOutNew3.nValue = 0;
            txOutNew3.scriptPubKey = outScript3;

            mutTx.vout.emplace_back(txOutNew3);
        }

        // Add the inputs
        mutTx.vin.emplace_back(in);
        mutTx.vin.emplace_back(in2);
        mutTx.vin.emplace_back(in3);

        CTransaction tx(mutTx);
        CValidationState state;

        // The inputs are spending 3000 Tokens (1000 of each ALPHACON, ALPHACONTEST, ALPHACONTESTTEST)
        // The outputs are spending 100 Tokens to 10 destinations (10 * 100 = 1000) (of each ALPHACON, ALPHACONTEST, ALPHACONTESTTEST)
        // This test should pass because for each token that is spent. It is assigned a destination
        std::vector<std::pair<std::string, uint256>> vReissueTokens;
        BOOST_CHECK_MESSAGE(Consensus::CheckTxTokens(tx, state, coins, 100000, 100, vReissueTokens, true), "CheckTxTokens Failed");


        // Try it not but only spend 900 of each token instead of 1000
        CMutableTransaction mutTx2;

        // Create CTxOut for each token that spends 100 tokens 9 time = 900 token in total
        for (int i = 0; i < 9; i++)
        {
            // Add the first token
            CTokenTransfer outToken("ALPHACON", 100, 0);
            CScript outScript = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outToken.ConstructTransaction(outScript);

            CTxOut txOutNew;
            txOutNew.nValue = 0;
            txOutNew.scriptPubKey = outScript;

            mutTx2.vout.emplace_back(txOutNew);

            // Add the second token
            CTokenTransfer outToken2("ALPHACONTEST", 100, 0);
            CScript outScript2 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outToken2.ConstructTransaction(outScript2);

            CTxOut txOutNew2;
            txOutNew2.nValue = 0;
            txOutNew2.scriptPubKey = outScript2;

            mutTx2.vout.emplace_back(txOutNew2);

            // Add the third token
            CTokenTransfer outToken3("ALPHACONTESTTEST", 100, 0);
            CScript outScript3 = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
            outToken3.ConstructTransaction(outScript3);

            CTxOut txOutNew3;
            txOutNew3.nValue = 0;
            txOutNew3.scriptPubKey = outScript3;

            mutTx2.vout.emplace_back(txOutNew3);
        }

        // Add the inputs
        mutTx2.vin.emplace_back(in);
        mutTx2.vin.emplace_back(in2);
        mutTx2.vin.emplace_back(in3);

        CTransaction tx2(mutTx2);

        // Check the transaction that contains inputs that are spending 1000 Tokens for 3 different tokens
        // While only outputs only contain 900 Tokens being sent to a destination
        // This should fail because 100 of each Token isn't being sent to a destination (Trying to burn 100 Tokens each)
        BOOST_CHECK_MESSAGE(!Consensus::CheckTxTokens(tx2, state, coins, 100000, 100, vReissueTokens, true), "CheckTxTokens should of failed");
    }

    BOOST_AUTO_TEST_CASE(token_tx_issue_units_test)
    {
        BOOST_TEST_MESSAGE("Running Token TX Issue Units Test");

        std::string error;
        CTokensCache cache;

        // Amount = 1.00000000
        CNewToken token("TOKEN", CAmount(100000000), 8, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test1: " + error);

        // Amount = 1.00000000
        token = CNewToken("TOKEN", CAmount(100000000), 0, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test2: " + error);

        // Amount = 0.10000000
        token = CNewToken("TOKEN", CAmount(10000000), 8, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test3: " + error);

        // Amount = 0.10000000
        token = CNewToken("TOKEN", CAmount(10000000), 2, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test4: " + error);

        // Amount = 0.10000000
        token = CNewToken("TOKEN", CAmount(10000000), 0, false, false, "");
        BOOST_CHECK_MESSAGE(!token.IsValid(error, cache, false, false), "Test5: " + error);

        // Amount = 0.01000000
        token = CNewToken("TOKEN", CAmount(1000000), 0, false, false, "");
        BOOST_CHECK_MESSAGE(!token.IsValid(error, cache, false, false), "Test6: " + error);

        // Amount = 0.01000000
        token = CNewToken("TOKEN", CAmount(1000000), 1, false, false, "");
        BOOST_CHECK_MESSAGE(!token.IsValid(error, cache, false, false), "Test7: " + error);

        // Amount = 0.01000000
        token = CNewToken("TOKEN", CAmount(1000000), 2, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test8: " + error);

        // Amount = 0.00000001
        token = CNewToken("TOKEN", CAmount(1), 8, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test9: " + error);

        // Amount = 0.00000010
        token = CNewToken("TOKEN", CAmount(10), 7, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test10: " + error);

        // Amount = 0.00000001
        token = CNewToken("TOKEN", CAmount(1), 7, false, false, "");
        BOOST_CHECK_MESSAGE(!token.IsValid(error, cache, false, false), "Test11: " + error);

        // Amount = 0.00000100
        token = CNewToken("TOKEN", CAmount(100), 6, false, false, "");
        BOOST_CHECK_MESSAGE(token.IsValid(error, cache, false, false), "Test12: " + error);

        // Amount = 0.00000100
        token = CNewToken("TOKEN", CAmount(100), 5, false, false, "");
        BOOST_CHECK_MESSAGE(!token.IsValid(error, cache, false, false), "Test13: " + error);
    }

BOOST_AUTO_TEST_SUITE_END()