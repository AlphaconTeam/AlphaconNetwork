// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACONCOIN_TOKENFILTERPROXY_H
#define ALPHACONCOIN_TOKENFILTERPROXY_H

#include <QSortFilterProxyModel>

class TokenFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TokenFilterProxy(QObject *parent = 0);

    void setTokenNamePrefix(const QString &tokenNamePrefix);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const;

private:
    QString tokenNamePrefix;
};


#endif //ALPHACONCOIN_TOKENFILTERPROXY_H
