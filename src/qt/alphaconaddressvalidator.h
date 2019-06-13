// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACON_QT_ALPHACONADDRESSVALIDATOR_H
#define ALPHACON_QT_ALPHACONADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class AlphaconAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit AlphaconAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Alphacon address widget validator, checks for a valid alphacon address.
 */
class AlphaconAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit AlphaconAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // ALPHACON_QT_ALPHACONADDRESSVALIDATOR_H
