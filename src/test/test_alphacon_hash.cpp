// Copyright (c) 2009-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <arith_uint256.h>
#include <hash.h>
#include <stdio.h>
#include <string.h>
#include <utilstrencodings.h>

int main(int argc, char **argv)
{
    if (argc == 2)
    {
        std::vector<unsigned char> rawHeader = ParseHex(argv[1]);
        std::cout << groestlhash(rawHeader.data(), rawHeader.data() + 80).GetHex();
    } else
    {
        std::cerr << "Usage: test_alphacon_hash blockHex" << std::endl;
        return 1;
    }

    return 0;
}