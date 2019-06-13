// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <tokens/tokens.h>

#include <test/test_alphacon.h>

#include <boost/test/unit_test.hpp>
#include "core_write.cpp"

#include <amount.h>
#include <base58.h>
#include <chainparams.h>

BOOST_FIXTURE_TEST_SUITE(token_tests, BasicTestingSetup)

    BOOST_AUTO_TEST_CASE(unit_validation_tests)
    {
        BOOST_TEST_MESSAGE("Running Unit Validation Test");

        BOOST_CHECK(IsTokenUnitsValid(COIN));
        BOOST_CHECK(IsTokenUnitsValid(CENT));
    }

    BOOST_AUTO_TEST_CASE(name_validation_tests)
    {
        BOOST_TEST_MESSAGE("Running Name Validation Test");

        TokenType type;

        // regular
        BOOST_CHECK(IsTokenNameValid("MIN", type));
        BOOST_CHECK(type == TokenType::ROOT);
        BOOST_CHECK(IsTokenNameValid("MAX_TOKEN_IS_30_CHARACTERS_LNG", type));
        BOOST_CHECK(!IsTokenNameValid("MAX_TOKEN_IS_31_CHARACTERS_LONG", type));
        BOOST_CHECK(type == TokenType::INVALID);
        BOOST_CHECK(IsTokenNameValid("A_BCDEFGHIJKLMNOPQRSTUVWXY.Z", type));
        BOOST_CHECK(IsTokenNameValid("0_12345678.9", type));

        BOOST_CHECK(!IsTokenNameValid("NO", type));
        BOOST_CHECK(!IsTokenNameValid("nolower", type));
        BOOST_CHECK(!IsTokenNameValid("NO SPACE", type));
        BOOST_CHECK(!IsTokenNameValid("(#&$(&*^%$))", type));

        BOOST_CHECK(!IsTokenNameValid("_ABC", type));
        BOOST_CHECK(!IsTokenNameValid("_ABC", type));
        BOOST_CHECK(!IsTokenNameValid("ABC_", type));
        BOOST_CHECK(!IsTokenNameValid(".ABC", type));
        BOOST_CHECK(!IsTokenNameValid("ABC.", type));
        BOOST_CHECK(!IsTokenNameValid("AB..C", type));
        BOOST_CHECK(!IsTokenNameValid("A__BC", type));
        BOOST_CHECK(!IsTokenNameValid("A._BC", type));
        BOOST_CHECK(!IsTokenNameValid("AB_.C", type));

        //- Versions of ALPHACONCOIN NOT allowed
        BOOST_CHECK(!IsTokenNameValid("ALP", type));
        BOOST_CHECK(!IsTokenNameValid("ALPHACON", type));
        BOOST_CHECK(!IsTokenNameValid("ALPHACONCOIN", type));

        //- Versions of ALPHACONCOIN ALLOWED
        BOOST_CHECK(IsTokenNameValid("ALPHACON.COIN", type));
        BOOST_CHECK(IsTokenNameValid("ALPHACON_COIN", type));
        BOOST_CHECK(IsTokenNameValid("ALPSPYDER", type));
        BOOST_CHECK(IsTokenNameValid("SPYDERALP", type));
        BOOST_CHECK(IsTokenNameValid("ALPHACONSPYDER", type));
        BOOST_CHECK(IsTokenNameValid("SPYDEALPHACON", type));
        BOOST_CHECK(IsTokenNameValid("BLACK_ALPHACONS", type));
        BOOST_CHECK(IsTokenNameValid("SEALPOT", type));

        // subs
        BOOST_CHECK(IsTokenNameValid("ABC/A", type));
        BOOST_CHECK(type == TokenType::SUB);
        BOOST_CHECK(IsTokenNameValid("ABC/A/1", type));
        BOOST_CHECK(IsTokenNameValid("ABC/A_1/1.A", type));
        BOOST_CHECK(IsTokenNameValid("ABC/AB/XYZ/STILL/MAX/30/123456", type));

        BOOST_CHECK(!IsTokenNameValid("ABC//MIN_1", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/NOTRAIL/", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/_X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/X_", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/.X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/X.", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/X__X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/X..X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/X_.X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/X._X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/nolower", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/NO SPACE", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/(*#^&$%)", type));
        BOOST_CHECK(!IsTokenNameValid("ABC/AB/XYZ/STILL/MAX/30/OVERALL/1234", type));

        // unique
        BOOST_CHECK(IsTokenNameValid("ABC#AZaz09", type));
        BOOST_CHECK(type == TokenType::UNIQUE);
        BOOST_CHECK(IsTokenNameValid("ABC#abc123ABC@$%&*()[]{}-_.?:", type));
        BOOST_CHECK(!IsTokenNameValid("ABC#no!bangs", type));
        BOOST_CHECK(IsTokenNameValid("ABC/THING#_STILL_31_MAX-------_", type));

        BOOST_CHECK(!IsTokenNameValid("MIN#", type));
        BOOST_CHECK(!IsTokenNameValid("ABC#NO#HASH", type));
        BOOST_CHECK(!IsTokenNameValid("ABC#NO SPACE", type));
        BOOST_CHECK(!IsTokenNameValid("ABC#RESERVED/", type));
        BOOST_CHECK(!IsTokenNameValid("ABC#RESERVED~", type));
        BOOST_CHECK(!IsTokenNameValid("ABC#RESERVED^", type));

        // channel
        BOOST_CHECK(IsTokenNameValid("ABC~1", type));
        BOOST_CHECK(type == TokenType::MSGCHANNEL);
        BOOST_CHECK(IsTokenNameValid("ABC~MAX_OF_12_CR", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~MAX_OF_12_CHR", type));
        BOOST_CHECK(IsTokenNameValid("TEST/TEST~CHANNEL", type));
        BOOST_CHECK(type == TokenType::MSGCHANNEL);

        BOOST_CHECK(!IsTokenNameValid("MIN~", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~NO~TILDE", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~_ANN", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~ANN_", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~.ANN", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~ANN.", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~X__X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~X._X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~X_.X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~X..X", type));
        BOOST_CHECK(!IsTokenNameValid("ABC~nolower", type));

        // owner
        BOOST_CHECK(IsTokenNameAnOwner("ABC!"));
        BOOST_CHECK(!IsTokenNameAnOwner("ABC"));
        BOOST_CHECK(!IsTokenNameAnOwner("ABC!COIN"));
        BOOST_CHECK(IsTokenNameAnOwner("MAX_TOKEN_IS_30_CHARACTERS_LNG!"));
        BOOST_CHECK(!IsTokenNameAnOwner("MAX_TOKEN_IS_31_CHARACTERS_LONG!"));
        BOOST_CHECK(IsTokenNameAnOwner("ABC/A!"));
        BOOST_CHECK(IsTokenNameAnOwner("ABC/A/1!"));
        BOOST_CHECK(IsTokenNameValid("ABC!", type));
        BOOST_CHECK(type == TokenType::OWNER);

        // vote
        BOOST_CHECK(IsTokenNameValid("ABC^VOTE"));
        BOOST_CHECK(!IsTokenNameValid("ABC^"));
        BOOST_CHECK(IsTokenNameValid("ABC^VOTING"));
        BOOST_CHECK(IsTokenNameValid("ABC^VOTING_IS_30_CHARACTERS_LN"));
        BOOST_CHECK(!IsTokenNameValid("ABC^VOTING_IS_31_CHARACTERS_LN!"));
        BOOST_CHECK(IsTokenNameValid("ABC/SUB/SUB/SUB/SUB^VOTE"));
        BOOST_CHECK(IsTokenNameValid("ABC/SUB/SUB/SUB/SUB/SUB/30^VOT"));
        BOOST_CHECK(IsTokenNameValid("ABC/SUB/SUB/SUB/SUB/SUB/31^VOTE"));
        BOOST_CHECK(!IsTokenNameValid("ABC/SUB/SUB/SUB/SUB/SUB/32X^VOTE"));
        BOOST_CHECK(IsTokenNameValid("ABC/SUB/SUB^VOTE", type));
        BOOST_CHECK(type == TokenType::VOTE);

        // Check type for different type of sub tokens
        BOOST_CHECK(IsTokenNameValid("TEST/UYTH#UNIQUE", type));
        BOOST_CHECK(type == TokenType::UNIQUE);

        BOOST_CHECK(IsTokenNameValid("TEST/UYTH/SUB#UNIQUE", type));
        BOOST_CHECK(type == TokenType::UNIQUE);

        BOOST_CHECK(IsTokenNameValid("TEST/UYTH/SUB~CHANNEL", type));
        BOOST_CHECK(type == TokenType::MSGCHANNEL);

        BOOST_CHECK(!IsTokenNameValid("TEST/UYTH/SUB#UNIQUE^VOTE", type));
        BOOST_CHECK(!IsTokenNameValid("TEST/UYTH/SUB#UNIQUE#UNIQUE", type));
        BOOST_CHECK(!IsTokenNameValid("TEST/UYTH/SUB~CHANNEL^VOTE", type));
        BOOST_CHECK(!IsTokenNameValid("TEST/UYTH/SUB~CHANNEL^UNIQUE", type));
        BOOST_CHECK(!IsTokenNameValid("TEST/UYTH/SUB~CHANNEL!", type));
        BOOST_CHECK(!IsTokenNameValid("TEST/UYTH/SUB^VOTE!", type));

        // Check ParentName function
        BOOST_CHECK(GetParentName("TEST!") == "TEST!");
        BOOST_CHECK(GetParentName("TEST") == "TEST");
        BOOST_CHECK(GetParentName("TEST/SUB") == "TEST");
        BOOST_CHECK(GetParentName("TEST/SUB#UNIQUE") == "TEST/SUB");
        BOOST_CHECK(GetParentName("TEST/TEST/SUB/SUB") == "TEST/TEST/SUB");
        BOOST_CHECK(GetParentName("TEST/SUB^VOTE") == "TEST/SUB");
        BOOST_CHECK(GetParentName("TEST/SUB/SUB~CHANNEL") == "TEST/SUB/SUB");
    }

    BOOST_AUTO_TEST_CASE(transfer_token_coin_test)
    {
        BOOST_TEST_MESSAGE("Running Transfer Token Coin Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create the token scriptPubKey
        CTokenTransfer token("ALPHACON", 1000);
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token.ConstructTransaction(scriptPubKey);

        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;


        Coin coin(txOut, 0, 0);

        BOOST_CHECK_MESSAGE(coin.IsToken(), "Transfer Token Coin isn't as token");
    }

    BOOST_AUTO_TEST_CASE(new_token_coin_test)
    {
        BOOST_TEST_MESSAGE("Running Token Coin Test");

        SelectParams(CBaseChainParams::MAIN);

        // Create the token scriptPubKey
        CNewToken token("ALPHACON", 1000, 8, 1, 0, "");
        CScript scriptPubKey = GetScriptForDestination(DecodeDestination(Params().GlobalBurnAddress()));
        token.ConstructTransaction(scriptPubKey);

        CTxOut txOut;
        txOut.nValue = 0;
        txOut.scriptPubKey = scriptPubKey;

        Coin coin(txOut, 0, 0);

        BOOST_CHECK_MESSAGE(coin.IsToken(), "New Token Coin isn't as token");
    }

    BOOST_AUTO_TEST_CASE(dwg_version_test)
    {
        BOOST_TEST_MESSAGE("Running DWG Version Test");

        int32_t version = 0x30000000;
        int32_t mask = 0xF0000000;
        int32_t new_version = version & mask;
        int32_t shifted = new_version >> 28;

        BOOST_CHECK_MESSAGE(shifted == 3, "New version didn't equal 3");
    }

    BOOST_AUTO_TEST_CASE(token_formatting_test)
    {
        BOOST_TEST_MESSAGE("Running Token Formatting Test");

        CAmount amount = 50000010000;
        BOOST_CHECK(ValueFromAmountString(amount, 4) == "500.0001");

        amount = 100;
        BOOST_CHECK(ValueFromAmountString(amount, 6) == "0.000001");

        amount = 1000;
        BOOST_CHECK(ValueFromAmountString(amount, 6) == "0.000010");

        amount = 50010101010;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "500.10101010");

        amount = 111111111;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "1.11111111");

        amount = 1;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "0.00000001");

        amount = 40000000;
        BOOST_CHECK(ValueFromAmountString(amount, 8) == "0.40000000");

    }

BOOST_AUTO_TEST_SUITE_END()
