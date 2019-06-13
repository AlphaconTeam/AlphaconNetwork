// Copyright (c) 2011-2015 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACON_QT_SENDTOKENSENTRY_H
#define ALPHACON_QT_SENDTOKENSENTRY_H

#include "walletmodel.h"

#include <QStackedWidget>

class WalletModel;
class PlatformStyle;
class QStringListModel;
class QSortFilterProxyModel;
class QCompleter;

namespace Ui {
    class SendTokensEntry;
}

/**
 * A single entry in the dialog for sending alphacons.
 * Stacked widget, with different UIs for payment requests
 * with a strong payee identity.
 */
class SendTokensEntry : public QStackedWidget
{
    Q_OBJECT

public:
    explicit SendTokensEntry(const PlatformStyle *platformStyle, const QStringList myTokensNames, QWidget *parent = 0);
    ~SendTokensEntry();

    void setModel(WalletModel *model);
    bool validate();
    SendTokensRecipient getValue();

    /** Return whether the entry is still empty and unedited */
    bool isClear();

    void setValue(const SendTokensRecipient &value);
    void setAddress(const QString &address);
    void CheckOwnerBox();
    void IsTokenControl(bool fIsTokenControl, bool fIsOwner);
    void setCurrentIndex(int index);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases
     *  (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setFocus();
    void setFocusTokenListBox();

    bool fUsingTokenControl;
    bool fShowAdministratorList;

    void refreshTokenList();
    void switchAdministratorList(bool fSwitchStatus = true);

    QStringListModel* stringModel;
    QSortFilterProxyModel* proxy;
    QCompleter* completer;


public Q_SLOTS:
    void clear();

Q_SIGNALS:
    void removeEntry(SendTokensEntry *entry);
    void payAmountChanged();
    void subtractFeeFromAmountChanged();

private Q_SLOTS:
    void deleteClicked();
    void on_payTo_textChanged(const QString &address);
    void on_addressBookButton_clicked();
    void on_pasteButton_clicked();
    void onTokenSelected(int index);
    void onSendOwnershipChanged();

private:
    SendTokensRecipient recipient;
    Ui::SendTokensEntry *ui;
    WalletModel *model;
    const PlatformStyle *platformStyle;

    bool updateLabel(const QString &address);
};

#endif // ALPHACON_QT_SENDTOKENSENTRY_H
