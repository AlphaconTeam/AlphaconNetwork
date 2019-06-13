// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tokensdialog.h"
#include "sendcoinsdialog.h"
#include "ui_tokensdialog.h"

#include "addresstablemodel.h"
#include "alphaconunits.h"
#include "clientmodel.h"
#include "tokencontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "sendtokensentry.h"
#include "walletmodel.h"
#include "tokentablemodel.h"

#include "base58.h"
#include "chainparams.h"
#include "wallet/coincontrol.h"
#include "validation.h" // mempool and minRelayTxFee
#include "ui_interface.h"
#include "txmempool.h"
#include "policy/fees.h"
#include "wallet/fees.h"
#include "createtokendialog.h"
#include "reissuetokendialog.h"
#include "guiconstants.h"

#include <QFontMetrics>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QTextDocument>
#include <QTimer>
#include <policy/policy.h>
#include <core_io.h>
#include <rpc/mining.h>

TokensDialog::TokensDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
        QDialog(parent),
        ui(new Ui::TokensDialog),
        clientModel(0),
        model(0),
        fNewRecipientAllowed(true),
        fFeeMinimized(true),
        platformStyle(_platformStyle)
{
    ui->setupUi(this);

    if (!_platformStyle->getImagesOnButtons()) {
        ui->addButton->setIcon(QIcon());
        ui->clearButton->setIcon(QIcon());
        ui->sendButton->setIcon(QIcon());
    } else {
        ui->addButton->setIcon(_platformStyle->SingleColorIcon(":/icons/add"));
        ui->clearButton->setIcon(_platformStyle->SingleColorIcon(":/icons/remove"));
        ui->sendButton->setIcon(_platformStyle->SingleColorIcon(":/icons/send"));
    }

    GUIUtil::setupAddressWidget(ui->lineEditTokenControlChange, this);

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    connect(ui->pushButtonTokenControl, SIGNAL(clicked()), this, SLOT(tokenControlButtonClicked()));
    connect(ui->checkBoxTokenControlChange, SIGNAL(stateChanged(int)), this, SLOT(tokenControlChangeChecked(int)));
    connect(ui->lineEditTokenControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(tokenControlChangeEdited(const QString &)));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(tokenControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(tokenControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(tokenControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(tokenControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(tokenControlClipboardBytes()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(tokenControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(tokenControlClipboardChange()));
    ui->labelTokenControlQuantity->addAction(clipboardQuantityAction);
    ui->labelTokenControlAmount->addAction(clipboardAmountAction);
    ui->labelTokenControlFee->addAction(clipboardFeeAction);
    ui->labelTokenControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelTokenControlBytes->addAction(clipboardBytesAction);
    ui->labelTokenControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelTokenControlChange->addAction(clipboardChangeAction);

    // init transaction fee section
    QSettings settings;
    if (!settings.contains("fFeeSectionMinimized"))
        settings.setValue("fFeeSectionMinimized", true);
    if (!settings.contains("nFeeRadio") && settings.contains("nTransactionFee") && settings.value("nTransactionFee").toLongLong() > 0) // compatibility
        settings.setValue("nFeeRadio", 1); // custom
    if (!settings.contains("nFeeRadio"))
        settings.setValue("nFeeRadio", 0); // recommended
    if (!settings.contains("nSmartFeeSliderPosition"))
        settings.setValue("nSmartFeeSliderPosition", 0);
    if (!settings.contains("nTransactionFee"))
        settings.setValue("nTransactionFee", (qint64)DEFAULT_TRANSACTION_FEE);
    if (!settings.contains("fPayOnlyMinFee"))
        settings.setValue("fPayOnlyMinFee", false);
    ui->groupFee->setId(ui->radioSmartFee, 0);
    ui->groupFee->setId(ui->radioCustomFee, 1);
    ui->groupFee->button((int)std::max(0, std::min(1, settings.value("nFeeRadio").toInt())))->setChecked(true);
    ui->customFee->setValue(settings.value("nTransactionFee").toLongLong());
    ui->checkBoxMinimumFee->setChecked(settings.value("fPayOnlyMinFee").toBool());
    minimizeFeeSection(settings.value("fFeeSectionMinimized").toBool());

    /** TOKENS START */
    setupTokenControlFrame(platformStyle);
    setupScrollView(platformStyle);
    setupFeeControl(platformStyle);
    /** TOKENS END */
}

void TokensDialog::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));
    }
}

void TokensDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(_model);
            }
        }

        setBalance(_model->getBalance(), _model->getUnconfirmedBalance(), _model->getImmatureBalance(),
                   _model->getWatchBalance(), _model->getWatchUnconfirmedBalance(), _model->getWatchImmatureBalance());
        connect(_model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(tokenControlUpdateLabels()));
        connect(_model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(tokenControlFeatureChanged(bool)));

        // Custom Fee Control
        connect(_model->getOptionsModel(), SIGNAL(customFeeFeaturesChanged(bool)), this, SLOT(customFeeFeatureChanged(bool)));


        ui->frameTokenControl->setVisible(false);
        ui->frameTokenControl->setVisible(_model->getOptionsModel()->getCoinControlFeatures());
        ui->frameFee->setVisible(_model->getOptionsModel()->getCustomFeeFeatures());
        tokenControlUpdateLabels();

        // fee section
        for (const int &n : confTargets) {
            ui->confTargetSelector->addItem(tr("%1 (%2 blocks)").arg(GUIUtil::formatNiceTimeOffset(n*Params().GetConsensus().nTargetSpacing)).arg(n));
        }
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(tokenControlUpdateLabels()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(tokenControlUpdateLabels()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(tokenControlUpdateLabels()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(tokenControlUpdateLabels()));
//        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(updateSmartFeeLabel()));
//        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(tokenControlUpdateLabels()));
        ui->customFee->setSingleStep(GetRequiredFee(1000));
        updateFeeSectionControls();
        updateMinFeeLabel();
        updateSmartFeeLabel();

        // set default rbf checkbox state
//        ui->optInRBF->setCheckState(model->getDefaultWalletRbf() ? Qt::Checked : Qt::Unchecked);
        ui->optInRBF->hide();

        // set the smartfee-sliders default value (wallets default conf.target or last stored value)
        QSettings settings;
        if (settings.value("nSmartFeeSliderPosition").toInt() != 0) {
            // migrate nSmartFeeSliderPosition to nConfTarget
            // nConfTarget is available since 0.15 (replaced nSmartFeeSliderPosition)
            int nConfirmTarget = 25 - settings.value("nSmartFeeSliderPosition").toInt(); // 25 == old slider range
            settings.setValue("nConfTarget", nConfirmTarget);
            settings.remove("nSmartFeeSliderPosition");
        }
        if (settings.value("nConfTarget").toInt() == 0)
            ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(model->getDefaultConfirmTarget()));
        else
            ui->confTargetSelector->setCurrentIndex(getIndexForConfTarget(settings.value("nConfTarget").toInt()));
    }
}

TokensDialog::~TokensDialog()
{
    QSettings settings;
    settings.setValue("fFeeSectionMinimized", fFeeMinimized);
    settings.setValue("nFeeRadio", ui->groupFee->checkedId());
    settings.setValue("nConfTarget", getConfTargetForIndex(ui->confTargetSelector->currentIndex()));
    settings.setValue("nTransactionFee", (qint64)ui->customFee->value());
    settings.setValue("fPayOnlyMinFee", ui->checkBoxMinimumFee->isChecked());

    delete ui;
}

void TokensDialog::setupTokenControlFrame(const PlatformStyle *platformStyle)
{
    /** Update the tokencontrol frame */
    ui->frameTokenControl->setStyleSheet(QString(".QFrame {background-color: %1; padding-top: 10px; padding-right: 5px; border: none;}").arg(platformStyle->WidgetBackGroundColor().name()));
    ui->widgetTokenControl->setStyleSheet(".QWidget {background-color: transparent;}");
    /** Create the shadow effects on the frames */

    ui->labelTokenControlFeatures->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlFeatures->setFont(GUIUtil::getTopLabelFont());

    ui->labelTokenControlQuantityText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlQuantityText->setFont(GUIUtil::getSubLabelFont());

    ui->labelTokenControlAmountText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlAmountText->setFont(GUIUtil::getSubLabelFont());

    ui->labelTokenControlFeeText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlFeeText->setFont(GUIUtil::getSubLabelFont());

    ui->labelTokenControlAfterFeeText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlAfterFeeText->setFont(GUIUtil::getSubLabelFont());

    ui->labelTokenControlBytesText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlBytesText->setFont(GUIUtil::getSubLabelFont());

    ui->labelTokenControlLowOutputText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlLowOutputText->setFont(GUIUtil::getSubLabelFont());

    ui->labelTokenControlChangeText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTokenControlChangeText->setFont(GUIUtil::getSubLabelFont());

    // Align the other labels next to the input buttons to the text in the same height
    ui->labelTokenControlAutomaticallySelected->setStyleSheet(STRING_LABEL_COLOR);

    // Align the Custom change address checkbox
    ui->checkBoxTokenControlChange->setStyleSheet(QString(".QCheckBox{ %1; }").arg(STRING_LABEL_COLOR));

    ui->labelTokenControlQuantity->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlAmount->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlFee->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlAfterFee->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlBytes->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlLowOutput->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlChange->setFont(GUIUtil::getSubLabelFont());
    ui->checkBoxTokenControlChange->setFont(GUIUtil::getSubLabelFont());
    ui->lineEditTokenControlChange->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlInsuffFunds->setFont(GUIUtil::getSubLabelFont());
    ui->labelTokenControlAutomaticallySelected->setFont(GUIUtil::getSubLabelFont());

}

void TokensDialog::setupScrollView(const PlatformStyle *platformStyle)
{
    /** Update the scrollview*/
    ui->scrollArea->setStyleSheet(QString(".QScrollArea{background-color: %1; border: none}").arg(platformStyle->WidgetBackGroundColor().name()));

    // Add some spacing so we can see the whole card
    ui->entries->setContentsMargins(10,10,20,0);
    ui->scrollAreaWidgetContents->setStyleSheet(QString(".QWidget{ background-color: %1;}").arg(platformStyle->WidgetBackGroundColor().name()));
}

void TokensDialog::setupFeeControl(const PlatformStyle *platformStyle)
{
    /** Update the coincontrol frame */
    ui->frameFee->setStyleSheet(QString(".QFrame {background-color: %1; padding-top: 10px; padding-right: 5px; border: none;}").arg(platformStyle->WidgetBackGroundColor().name()));
    /** Create the shadow effects on the frames */

    ui->labelFeeHeadline->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelFeeHeadline->setFont(GUIUtil::getSubLabelFont());

    ui->labelSmartFee3->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCustomPerKilobyte->setStyleSheet(QString(".QLabel{ %1; }").arg(STRING_LABEL_COLOR));
    ui->radioSmartFee->setStyleSheet(STRING_LABEL_COLOR);
    ui->radioCustomFee->setStyleSheet(STRING_LABEL_COLOR);
    ui->checkBoxMinimumFee->setStyleSheet(QString(".QCheckBox{ %1; }").arg(STRING_LABEL_COLOR));

    ui->buttonChooseFee->setFont(GUIUtil::getSubLabelFont());
    ui->fallbackFeeWarningLabel->setFont(GUIUtil::getSubLabelFont());
    ui->buttonMinimizeFee->setFont(GUIUtil::getSubLabelFont());
    ui->radioSmartFee->setFont(GUIUtil::getSubLabelFont());
    ui->labelSmartFee2->setFont(GUIUtil::getSubLabelFont());
    ui->labelSmartFee3->setFont(GUIUtil::getSubLabelFont());
    ui->confTargetSelector->setFont(GUIUtil::getSubLabelFont());
    ui->radioCustomFee->setFont(GUIUtil::getSubLabelFont());
    ui->labelCustomPerKilobyte->setFont(GUIUtil::getSubLabelFont());
    ui->customFee->setFont(GUIUtil::getSubLabelFont());
    ui->labelMinFeeWarning->setFont(GUIUtil::getSubLabelFont());
    ui->optInRBF->setFont(GUIUtil::getSubLabelFont());
    ui->sendButton->setFont(GUIUtil::getSubLabelFont());
    ui->clearButton->setFont(GUIUtil::getSubLabelFont());
    ui->addButton->setFont(GUIUtil::getSubLabelFont());
    ui->labelSmartFee->setFont(GUIUtil::getSubLabelFont());
    ui->labelFeeEstimation->setFont(GUIUtil::getSubLabelFont());
    ui->labelFeeMinimized->setFont(GUIUtil::getSubLabelFont());

}

void TokensDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;

    QList<SendTokensRecipient> recipients;
    bool valid = true;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
    }

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    fNewRecipientAllowed = false;
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    std::vector< std::pair<CTokenTransfer, std::string> >vTransfers;

    for (auto recipient : recipients) {
        vTransfers.emplace_back(std::make_pair(CTokenTransfer(recipient.tokenName.toStdString(), recipient.amount, recipient.tokenLockTime), recipient.address.toStdString()));
    }

    // Always use a CCoinControl instance, use the TokenControlDialog instance if CoinControl has been enabled
    CCoinControl ctrl;
    if (model->getOptionsModel()->getCoinControlFeatures())
        ctrl = *TokenControlDialog::tokenControl;

    updateTokenControlState(ctrl);

    CWalletTx tx;
    CReserveKey reservekey(model->getWallet());
    std::pair<int, std::string> error;
    CAmount nFeeRequired;

    if (!CreateTransferTokenTransaction(model->getWallet(), ctrl, vTransfers, "", error, tx, reservekey, nFeeRequired)) {
        QMessageBox msgBox;
        msgBox.setText(QString::fromStdString(error.second));
        msgBox.exec();
        return;
    }

    // Format confirmation message
    QStringList formatted;
    for (SendTokensRecipient &rcp : recipients)
    {
        // generate bold amount string
        QString amount = "<b>" + QString::fromStdString(ValueFromAmountString(rcp.amount, 8)) + " " + rcp.tokenName;
        amount.append("</b>");
        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty()) // authenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else // unauthenticated payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        if (rcp.tokenLockTime > 0) {
            recipientElement.append(QString(" with lock time %1").arg(rcp.tokenLockTime));
        }

        formatted.append(recipientElement);
    }

    QString questionString = tr("Are you sure you want to send?");
    questionString.append("<br /><br />%1");

    if(nFeeRequired > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(AlphaconUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), nFeeRequired));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));

        // append transaction size
        questionString.append(" (" + QString::number((double)GetVirtualTransactionSize(tx) / 1000) + " kB)");
    }

//    if (ui->optInRBF->isChecked())
//    {
//        questionString.append("<hr /><span>");
//        questionString.append(tr("This transaction signals replaceability (optin-RBF)."));
//        questionString.append("</span>");
//    }

    SendConfirmationDialog confirmationDialog(tr("Confirm send tokens"),
                                              questionString.arg(formatted.join("<br />")), SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    if(retval != QMessageBox::Yes)
    {
        fNewRecipientAllowed = true;
        return;
    }

    // now send the prepared transaction
    WalletModel::SendCoinsReturn sendStatus = model->sendTokens(tx, recipients, reservekey);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        TokenControlDialog::tokenControl->UnSelectAll();
        tokenControlUpdateLabels();
        accept();
    }
    fNewRecipientAllowed = true;
}

void TokensDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void TokensDialog::reject()
{
    clear();
}

void TokensDialog::accept()
{
    clear();
}

SendTokensEntry *TokensDialog::addEntry()
{
    LOCK(cs_main);
    std::vector<std::string> tokens;
    if (model)
        GetAllMyTokens(model->getWallet(), tokens, 0);

    QStringList list;
    bool fIsOwner = false;
    bool fIsTokenControl = false;
    if (TokenControlDialog::tokenControl->HasTokenSelected()) {
        list << QString::fromStdString(TokenControlDialog::tokenControl->strTokenSelected);
        fIsOwner = IsTokenNameAnOwner(TokenControlDialog::tokenControl->strTokenSelected);
        fIsTokenControl = true;
    } else {
        for (auto name : tokens) {
            if (!IsTokenNameAnOwner(name))
                list << QString::fromStdString(name);
        }
    }

    SendTokensEntry *entry = new SendTokensEntry(platformStyle, list, this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendTokensEntry*)), this, SLOT(removeEntry(SendTokensEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(tokenControlUpdateLabels()));
    connect(entry, SIGNAL(subtractFeeFromAmountChanged()), this, SLOT(tokenControlUpdateLabels()));

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocusTokenListBox();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());

    entry->IsTokenControl(fIsTokenControl, fIsOwner);

    if (list.size() == 1)
        entry->setCurrentIndex(1);

    updateTabsAndLabels();

    return entry;
}

void TokensDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    tokenControlUpdateLabels();
}

void TokensDialog::removeEntry(SendTokensEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *TokensDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void TokensDialog::setAddress(const QString &address)
{
    SendTokensEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendTokensEntry *first = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void TokensDialog::pasteEntry(const SendTokensRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendTokensEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendTokensEntry *first = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool TokensDialog::handlePaymentRequest(const SendTokensRecipient &rv)
{
    // Just paste the entry, all pre-checks
    // are done in paymentserver.cpp.
    pasteEntry(rv);
    return true;
}

void TokensDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                                 const CAmount& watchBalance, const CAmount& watchUnconfirmedBalance, const CAmount& watchImmatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);
    Q_UNUSED(watchBalance);
    Q_UNUSED(watchUnconfirmedBalance);
    Q_UNUSED(watchImmatureBalance);

    ui->labelBalance->setFont(GUIUtil::getSubLabelFont());
    ui->label->setFont(GUIUtil::getSubLabelFont());

    if(model && model->getOptionsModel())
    {
        ui->labelBalance->setText(AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void TokensDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0, 0, 0, 0);
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
}

void TokensDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
        case WalletModel::InvalidAddress:
            msgParams.first = tr("The recipient address is not valid. Please recheck.");
            break;
        case WalletModel::InvalidAmount:
            msgParams.first = tr("The amount to pay must be larger than 0.");
            break;
        case WalletModel::AmountExceedsBalance:
            msgParams.first = tr("The amount exceeds your balance.");
            break;
        case WalletModel::AmountWithFeeExceedsBalance:
            msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
            break;
        case WalletModel::DuplicateAddress:
            msgParams.first = tr("Duplicate address found: addresses should only be used once each.");
            break;
        case WalletModel::TransactionCreationFailed:
            msgParams.first = tr("Transaction creation failed!");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::TransactionCommitFailed:
            msgParams.first = tr("The transaction was rejected with the following reason: %1").arg(sendCoinsReturn.reasonCommitFailed);
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
        case WalletModel::AbsurdFee:
            msgParams.first = tr("A fee higher than %1 is considered an absurdly high fee.").arg(AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), maxTxFee));
            break;
        case WalletModel::PaymentRequestExpired:
            msgParams.first = tr("Payment request expired.");
            msgParams.second = CClientUIInterface::MSG_ERROR;
            break;
            // included to prevent a compiler warning.
        case WalletModel::OK:
        default:
            return;
    }

    Q_EMIT message(tr("Send Coins"), msgParams.first, msgParams.second);
}

void TokensDialog::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee  ->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void TokensDialog::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void TokensDialog::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void TokensDialog::setMinimumFee()
{
    ui->customFee->setValue(GetRequiredFee(1000));
}

void TokensDialog::updateFeeSectionControls()
{
    ui->confTargetSelector      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee           ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee2          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelSmartFee3          ->setEnabled(ui->radioSmartFee->isChecked());
    ui->labelFeeEstimation      ->setEnabled(ui->radioSmartFee->isChecked());
    ui->checkBoxMinimumFee      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelMinFeeWarning      ->setEnabled(ui->radioCustomFee->isChecked());
    ui->labelCustomPerKilobyte  ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
    ui->customFee               ->setEnabled(ui->radioCustomFee->isChecked() && !ui->checkBoxMinimumFee->isChecked());
}

void TokensDialog::updateFeeMinimizedLabel()
{
    if(!model || !model->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) + "/kB");
    }
}

void TokensDialog::updateMinFeeLabel()
{
    if (model && model->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
                AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), GetRequiredFee(1000)) + "/kB")
        );
}

void TokensDialog::updateTokenControlState(CCoinControl& ctrl)
{
    if (ui->radioCustomFee->isChecked()) {
        ctrl.m_feerate = CFeeRate(ui->customFee->value());
    } else {
        ctrl.m_feerate.reset();
    }
    // Avoid using global defaults when sending money from the GUI
    // Either custom fee will be used or if not selected, the confirmation target from dropdown box
    ctrl.m_confirm_target = getConfTargetForIndex(ui->confTargetSelector->currentIndex());
//    ctrl.signalRbf = ui->optInRBF->isChecked();
}

void TokensDialog::updateSmartFeeLabel()
{
    if(!model || !model->getOptionsModel())
        return;
    CCoinControl coin_control;
    updateTokenControlState(coin_control);
    coin_control.m_feerate.reset(); // Explicitly use only fee estimation rate for smart fee labels
    FeeCalculation feeCalc;
    CFeeRate feeRate = CFeeRate(GetMinimumFee(1000, coin_control, ::mempool, ::feeEstimator, &feeCalc));

    ui->labelSmartFee->setText(AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), feeRate.GetFeePerK()) + "/kB");

    if (feeCalc.reason == FeeReason::FALLBACK) {
        ui->labelSmartFee2->show(); // (Smart fee not initialized yet. This usually takes a few blocks...)
        ui->labelFeeEstimation->setText("");
        ui->fallbackFeeWarningLabel->setVisible(true);
        int lightness = ui->fallbackFeeWarningLabel->palette().color(QPalette::WindowText).lightness();
        QColor warning_colour(255 - (lightness / 5), 176 - (lightness / 3), 48 - (lightness / 14));
        ui->fallbackFeeWarningLabel->setStyleSheet("QLabel { color: " + warning_colour.name() + "; }");
        ui->fallbackFeeWarningLabel->setIndent(QFontMetrics(ui->fallbackFeeWarningLabel->font()).width("x"));
    }
    else
    {
        ui->labelSmartFee2->hide();
        ui->labelFeeEstimation->setText(tr("Estimated to begin confirmation within %n block(s).", "", feeCalc.returnedTarget));
        ui->fallbackFeeWarningLabel->setVisible(false);
    }

    updateFeeMinimizedLabel();
}

// Coin Control: copy label "Quantity" to clipboard
void TokensDialog::tokenControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelTokenControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void TokensDialog::tokenControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelTokenControlAmount->text().left(ui->labelTokenControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void TokensDialog::tokenControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelTokenControlFee->text().left(ui->labelTokenControlFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "After fee" to clipboard
void TokensDialog::tokenControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelTokenControlAfterFee->text().left(ui->labelTokenControlAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Bytes" to clipboard
void TokensDialog::tokenControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelTokenControlBytes->text().replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Dust" to clipboard
void TokensDialog::tokenControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelTokenControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void TokensDialog::tokenControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelTokenControlChange->text().left(ui->labelTokenControlChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void TokensDialog::tokenControlFeatureChanged(bool checked)
{
    ui->frameTokenControl->setVisible(checked);

    if (!checked && model) // coin control features disabled
        TokenControlDialog::tokenControl->SetNull();

    tokenControlUpdateLabels();
}

void TokensDialog::customFeeFeatureChanged(bool checked)
{
    ui->frameFee->setVisible(checked);
}

// Coin Control: button inputs -> show actual coin control dialog
void TokensDialog::tokenControlButtonClicked()
{
    TokenControlDialog dlg(platformStyle);
    dlg.setModel(model);
    dlg.exec();
    tokenControlUpdateLabels();
    tokenControlUpdateSendCoinsDialog();
}

// Coin Control: checkbox custom change address
void TokensDialog::tokenControlChangeChecked(int state)
{
    if (state == Qt::Unchecked)
    {
        TokenControlDialog::tokenControl->destChange = CNoDestination();
        ui->labelTokenControlChangeLabel->clear();
    }
    else
        // use this to re-validate an already entered address
        tokenControlChangeEdited(ui->lineEditTokenControlChange->text());

    ui->lineEditTokenControlChange->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void TokensDialog::tokenControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel())
    {
        // Default to no change address until verified
        TokenControlDialog::tokenControl->destChange = CNoDestination();
        ui->labelTokenControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        const CTxDestination dest = DecodeDestination(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelTokenControlChangeLabel->setText("");
        }
        else if (!IsValidDestination(dest)) // Invalid address
        {
            ui->labelTokenControlChangeLabel->setText(tr("Warning: Invalid Alphacon address"));
        }
        else // Valid address
        {
            if (!model->IsSpendable(dest)) {
                ui->labelTokenControlChangeLabel->setText(tr("Warning: Unknown change address"));

                // confirmation dialog
                QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm custom change address"), tr("The address you selected for change is not part of this wallet. Any or all funds in your wallet may be sent to this address. Are you sure?"),
                                                                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

                if(btnRetVal == QMessageBox::Yes)
                    TokenControlDialog::tokenControl->destChange = dest;
                else
                {
                    ui->lineEditTokenControlChange->setText("");
                    ui->labelTokenControlChangeLabel->setStyleSheet("QLabel{color:black;}");
                    ui->labelTokenControlChangeLabel->setText("");
                }
            }
            else // Known change address
            {
                ui->labelTokenControlChangeLabel->setStyleSheet("QLabel{color:black;}");

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                if (!associatedLabel.isEmpty())
                    ui->labelTokenControlChangeLabel->setText(associatedLabel);
                else
                    ui->labelTokenControlChangeLabel->setText(tr("(no label)"));

                TokenControlDialog::tokenControl->destChange = dest;
            }
        }
    }
}

// Coin Control: update labels
void TokensDialog::tokenControlUpdateLabels()
{
    if (!model || !model->getOptionsModel())
        return;

    updateTokenControlState(*TokenControlDialog::tokenControl);

    // set pay amounts
    TokenControlDialog::payAmounts.clear();
    TokenControlDialog::fSubtractFeeFromAmount = false;

    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(i)->widget());
        if(entry && !entry->isHidden())
        {
            SendTokensRecipient rcp = entry->getValue();
            TokenControlDialog::payAmounts.append(rcp.amount);
//            if (rcp.fSubtractFeeFromAmount)
//                TokenControlDialog::fSubtractFeeFromAmount = true;
        }
    }

    if (TokenControlDialog::tokenControl->HasTokenSelected())
    {
        // actual coin control calculation
        TokenControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelTokenControlAutomaticallySelected->hide();
        ui->widgetTokenControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelTokenControlAutomaticallySelected->show();
        ui->widgetTokenControl->hide();
        ui->labelTokenControlInsuffFunds->hide();
    }
}

/** TOKENS START */
void TokensDialog::tokenControlUpdateSendCoinsDialog()
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            removeEntry(entry);
        }
    }

    addEntry();

}

void TokensDialog::processNewTransaction()
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            entry->refreshTokenList();
        }
    }
}

void TokensDialog::focusToken(const QModelIndex &idx)
{

    clear();

    SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(0)->widget());
    if(entry)
    {
        SendTokensRecipient recipient;
        recipient.tokenName = idx.data(TokenTableModel::TokenNameRole).toString();

        entry->setValue(recipient);
        entry->setFocus();
    }
}

void TokensDialog::focusTokenListBox()
{

    SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(0)->widget());
    if (entry)
    {
        entry->setFocusTokenListBox();

        if (entry->getValue().tokenName != "")
            entry->setFocus();

    }
}

void TokensDialog::handleFirstSelection()
{
    SendTokensEntry *entry = qobject_cast<SendTokensEntry*>(ui->entries->itemAt(0)->widget());
    if (entry) {
        entry->refreshTokenList();
    }
}
/** TOKENS END */
