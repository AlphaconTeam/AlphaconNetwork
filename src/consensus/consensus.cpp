// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "consensus.h"
#include <validation.h>

unsigned int GetMaxBlockWeight()
{
    return MAX_BLOCK_WEIGHT;
}

unsigned int GetMaxBlockSerializedSize()
{
    return MAX_BLOCK_SERIALIZED_SIZE;
}