// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACON_QT_TOKENTABLEMODEL_H
#define ALPHACON_QT_TOKENTABLEMODEL_H

#include "amount.h"

#include <QAbstractTableModel>
#include <QStringList>

class TokenTablePriv;
class WalletModel;
class TokenRecord;

class CTokens;

/** Models tokens portion of wallet as table of owned tokens.
 */
class TokenTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit TokenTableModel(WalletModel *parent = 0);
    ~TokenTableModel();

    enum ColumnIndex {
        Name = 0,
        Quantity = 1
    };

    /** Roles to get specific information from a transaction row.
        These are independent of column.
    */
    enum RoleIndex {
        /** Net amount of transaction */
            AmountRole = 100,
        /** ALP or name of an token */
            TokenNameRole = 101,
        /** Formatted amount, without brackets when unconfirmed */
            FormattedAmountRole = 102,
        /** AdministratorRole */
            AdministratorRole = 103,
        /** IsLockedRole */
            IsLockedRole = 104
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    QString formatTooltip(const TokenRecord *rec) const;
    QString formatTokenName(const TokenRecord *wtx) const;
    QString formatTokenQuantity(const TokenRecord *wtx) const;

    void checkBalanceChanged();

private:
    WalletModel *walletModel;
    QStringList columns;
    TokenTablePriv *priv;

    friend class TokenTablePriv;
};

#endif // ALPHACON_QT_TOKENTABLEMODEL_H
