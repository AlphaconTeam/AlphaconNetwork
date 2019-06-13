// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACON_QT_TOKENSDIALOG_H
#define ALPHACON_QT_TOKENSDIALOG_H

#include "walletmodel.h"

#include <QDialog>
#include <QMessageBox>
#include <QString>
#include <QTimer>

class ClientModel;
class PlatformStyle;
class SendTokensEntry;
class SendCoinsRecipient;

namespace Ui {
    class TokensDialog;
}

QT_BEGIN_NAMESPACE
class QUrl;
QT_END_NAMESPACE

/** Dialog for sending alphacons */
class TokensDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TokensDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~TokensDialog();

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);
    void setupTokenControlFrame(const PlatformStyle *platformStyle);
    void setupScrollView(const PlatformStyle *platformStyle);
    void setupFeeControl(const PlatformStyle *platformStyle);

    /** Set up the tab chain manually, as Qt messes up the tab chain by default in some cases (issue https://bugreports.qt-project.org/browse/QTBUG-10907).
     */
    QWidget *setupTabChain(QWidget *prev);

    void setAddress(const QString &address);
    void pasteEntry(const SendTokensRecipient &rv);
    bool handlePaymentRequest(const SendTokensRecipient &recipient);
    void processNewTransaction();

    // The first time the transfer token screen is loaded, the wallet isn't doing loading so the token list is empty.
    // The first time the screen is navigated to, refresh the token list
    void handleFirstSelection();

public Q_SLOTS:
    void clear();
    void reject();
    void accept();
    SendTokensEntry *addEntry();
    void updateTabsAndLabels();
    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);
    void focusTokenListBox();

private:
    Ui::TokensDialog *ui;
    ClientModel *clientModel;
    WalletModel *model;
    bool fNewRecipientAllowed;
    bool fFeeMinimized;
    const PlatformStyle *platformStyle;

    // Process WalletModel::SendCoinsReturn and generate a pair consisting
    // of a message and message flags for use in Q_EMIT message().
    // Additional parameter msgArg can be used via .arg(msgArg).
    void processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg = QString());
    void minimizeFeeSection(bool fMinimize);
    void updateFeeMinimizedLabel();
    // Update the passed in CCoinControl with state from the GUI
    void updateTokenControlState(CCoinControl& ctrl);



private Q_SLOTS:
    void on_sendButton_clicked();
    void on_buttonChooseFee_clicked();
    void on_buttonMinimizeFee_clicked();
    void removeEntry(SendTokensEntry* entry);
    void updateDisplayUnit();
    void tokenControlFeatureChanged(bool);
    void tokenControlButtonClicked();
    void tokenControlChangeChecked(int);
    void tokenControlChangeEdited(const QString &);
    void tokenControlUpdateLabels();
    void tokenControlClipboardQuantity();
    void tokenControlClipboardAmount();
    void tokenControlClipboardFee();
    void tokenControlClipboardAfterFee();
    void tokenControlClipboardBytes();
    void tokenControlClipboardLowOutput();
    void tokenControlClipboardChange();
    void setMinimumFee();
    void updateFeeSectionControls();
    void updateMinFeeLabel();
    void updateSmartFeeLabel();

    void customFeeFeatureChanged(bool);

    /** TOKENS START */
    void tokenControlUpdateSendCoinsDialog();
    void focusToken(const QModelIndex& index);
    /** TOKENS END */

    Q_SIGNALS:
            // Fired when a message should be reported to the user
            void message(const QString &title, const QString &message, unsigned int style);
};

#endif // ALPHACON_QT_TOKENSSDIALOG_H
