// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACON_QT_REISSUETOKENDIALOG_H
#define ALPHACON_QT_REISSUETOKENDIALOG_H

#include "walletmodel.h"

#include <QDialog>

class PlatformStyle;
class WalletModel;
class ClientModel;
class CNewToken;
class QStringListModel;
class QSortFilterProxyModel;
class QCompleter;

namespace Ui {
    class ReissueTokenDialog;
}

QT_BEGIN_NAMESPACE
class QModelIndex;
QT_END_NAMESPACE

/** Dialog showing transaction details. */
class ReissueTokenDialog : public QDialog
{
Q_OBJECT

public:
    explicit ReissueTokenDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~ReissueTokenDialog();

    void setClientModel(ClientModel *clientModel);
    void setModel(WalletModel *model);

    QString formatGreen;
    QString formatBlack;

    void setupCoinControlFrame(const PlatformStyle *platformStyle);
    void setupTokenDataView(const PlatformStyle *platformStyle);
    void setupFeeControl(const PlatformStyle *platformStyle);
    void updateTokensList();

    void clear();

    QStringListModel* stringModel;
    QSortFilterProxyModel* proxy;
    QCompleter* completer;

private:
    Ui::ReissueTokenDialog *ui;
    ClientModel *clientModel;
    WalletModel *model;
    const PlatformStyle *platformStyle;
    bool fFeeMinimized;

    CNewToken *token;

    void toggleIPFSText();
    void setUpValues();
    void showMessage(QString string);
    void showValidMessage(QString string);
    void hideMessage();
    void disableReissueButton();
    void enableReissueButton();
    void CheckFormState();
    void disableAll();
    void enableDataEntry();
    void buildUpdatedData();
    void setDisplayedDataToNone();

    //CoinControl
    // Update the passed in CCoinControl with state from the GUI
    void updateCoinControlState(CCoinControl& ctrl);

    //Fee
    void updateFeeMinimizedLabel();
    void minimizeFeeSection(bool fMinimize);

    //Validation of IPFS
    bool checkIPFSHash(QString hash);

private Q_SLOTS:
    void onTokenSelected(int index);
    void onQuantityChanged(double qty);
    void onIPFSStateChanged();
    void onIPFSHashChanged(QString hash);
    void onAddressNameChanged(QString address);
    void onReissueTokenClicked();
    void onReissueBoxChanged();
    void onUnitChanged(int value);
    void onClearButtonClicked();

    //CoinControl
    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();
    void coinControlUpdateLabels();

    //Fee
    void on_buttonChooseFee_clicked();
    void on_buttonMinimizeFee_clicked();
    void setMinimumFee();
    void updateFeeSectionControls();
    void updateMinFeeLabel();
    void updateSmartFeeLabel();
    void feeControlFeatureChanged(bool);

    void setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                    const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance);
    void updateDisplayUnit();

    void focusReissueToken(const QModelIndex &index);

Q_SIGNALS:
    // Fired when a message should be reported to the user
    void message(const QString &title, const QString &message, unsigned int style);
};

#endif // ALPHACON_QT_REISSUETOKENDIALOG_H
