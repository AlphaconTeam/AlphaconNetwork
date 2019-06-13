
#include "tokens/tokens.h"
#include <boost/test/unit_test.hpp>
#include <test/test_alphacon.h>

BOOST_FIXTURE_TEST_SUITE(cache_tests, BasicTestingSetup)


    const int NUM_OF_TOKENS1 = 100000;

BOOST_AUTO_TEST_CASE(cache_test)
{
    BOOST_TEST_MESSAGE("Running Cache Test");

    CLRUCache<std::string, CNewToken> cache(NUM_OF_TOKENS1);

    std::string tokenName = "TEST";

    int counter = 0;
    while(cache.Size() != NUM_OF_TOKENS1)
    {
        CNewToken token(std::string(tokenName + std::to_string(counter)), CAmount(1), 0, 0, 1, "43f81c6f2c0593bde5a85e09ae662816eca80797");

        cache.Put(token.strName, token);
        counter++;
    }

    BOOST_CHECK_MESSAGE(cache.Exists("TEST0"), "Didn't have TEST0");

    CNewToken token("THISWILLOVERWRITE", CAmount(1), 0, 0, 1, "43f81c6f2c0593bde5a85e09ae662816eca80797");

    cache.Put(token.strName, token);

    BOOST_CHECK_MESSAGE(cache.Exists("THISWILLOVERWRITE"), "New token wasn't added to cache");
    BOOST_CHECK_MESSAGE(!cache.Exists("TEST0"), "Cache didn't remove the least recently used");
    BOOST_CHECK_MESSAGE(cache.Exists("TEST1"), "Cache didn't have TEST1");

}

BOOST_AUTO_TEST_SUITE_END()

