// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "createtokendialog.h"
#include "ui_createtokendialog.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "sendcoinsdialog.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "alphaconunits.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "guiconstants.h"

#include "wallet/coincontrol.h"
#include "policy/fees.h"
#include "wallet/fees.h"

#include <script/standard.h>
#include <base58.h>
#include <validation.h> // mempool and minRelayTxFee
#include <wallet/wallet.h>
#include <core_io.h>
#include <policy/policy.h>
#include "tokens/tokentypes.h"
#include "tokentablemodel.h"

#include <QModelIndex>
#include <QDebug>
#include <QMessageBox>
#include <QClipboard>
#include <QSettings>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QCompleter>

CreateTokenDialog::CreateTokenDialog(const PlatformStyle *_platformStyle, QWidget *parent) :
        QDialog(parent, Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint | Qt::WindowMaximizeButtonHint),
        ui(new Ui::CreateTokenDialog),
        platformStyle(_platformStyle)
{
    ui->setupUi(this);
    setWindowTitle("Create Tokens");
    connect(ui->ipfsBox, SIGNAL(clicked()), this, SLOT(ipfsStateChanged()));
    connect(ui->availabilityButton, SIGNAL(clicked()), this, SLOT(checkAvailabilityClicked()));
    connect(ui->nameText, SIGNAL(textChanged(QString)), this, SLOT(onNameChanged(QString)));
    connect(ui->addressText, SIGNAL(textChanged(QString)), this, SLOT(onAddressNameChanged(QString)));
    connect(ui->ipfsText, SIGNAL(textChanged(QString)), this, SLOT(onIPFSHashChanged(QString)));
    connect(ui->createTokenButton, SIGNAL(clicked()), this, SLOT(onCreateTokenClicked()));
    connect(ui->unitBox, SIGNAL(valueChanged(int)), this, SLOT(onUnitChanged(int)));
    connect(ui->tokenType, SIGNAL(activated(int)), this, SLOT(onTokenTypeActivated(int)));
    connect(ui->tokenList, SIGNAL(activated(int)), this, SLOT(onTokenListActivated(int)));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(onClearButtonClicked()));

    GUIUtil::setupAddressWidget(ui->lineEditCoinControlChange, this);

    // Coin Control
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy dust"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

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

    format = "%1<font color=green>%2%3</font>";

    setupCoinControlFrame(platformStyle);
    setupTokenDataView(platformStyle);
    setupFeeControl(platformStyle);

    /** Setup the token list combobox */
    stringModel = new QStringListModel;

    proxy = new QSortFilterProxyModel;
    proxy->setSourceModel(stringModel);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->tokenList->setModel(proxy);
    ui->tokenList->setEditable(true);
    ui->tokenList->lineEdit()->setPlaceholderText("Select an token");

    completer = new QCompleter(proxy,this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->tokenList->setCompleter(completer);

    ui->nameText->installEventFilter(this);
    ui->tokenList->installEventFilter(this);

    ui->ipfsBox->hide();
    ui->ipfsText->hide();
}

void CreateTokenDialog::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;

    if (_clientModel) {
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(updateSmartFeeLabel()));
    }
}

void CreateTokenDialog::setModel(WalletModel *_model)
{
    this->model = _model;

    if(_model && _model->getOptionsModel())
    {
        setBalance(_model->getBalance(), _model->getUnconfirmedBalance(), _model->getImmatureBalance(),
                   _model->getWatchBalance(), _model->getWatchUnconfirmedBalance(), _model->getWatchImmatureBalance());
        connect(_model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        updateDisplayUnit();

        // Coin Control
        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(_model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        bool fCoinControlEnabled = _model->getOptionsModel()->getCoinControlFeatures();
        ui->frameCoinControl->setVisible(fCoinControlEnabled);
        ui->addressText->setVisible(fCoinControlEnabled);
        ui->addressLabel->setVisible(fCoinControlEnabled);
        coinControlUpdateLabels();

        // Custom Fee Control
        ui->frameFee->setVisible(_model->getOptionsModel()->getCustomFeeFeatures());
        connect(_model->getOptionsModel(), SIGNAL(customFeeFeaturesChanged(bool)), this, SLOT(feeControlFeatureChanged(bool)));

        // fee section
        for (const int &n : confTargets) {
            ui->confTargetSelector->addItem(tr("%1 (%2 blocks)").arg(GUIUtil::formatNiceTimeOffset(n*Params().GetConsensus().nTargetSpacing)).arg(n));
        }
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSmartFeeLabel()));
        connect(ui->confTargetSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->groupFee, SIGNAL(buttonClicked(int)), this, SLOT(coinControlUpdateLabels()));
        connect(ui->customFee, SIGNAL(valueChanged()), this, SLOT(coinControlUpdateLabels()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(setMinimumFee()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(updateFeeSectionControls()));
        connect(ui->checkBoxMinimumFee, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
//        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(updateSmartFeeLabel()));
//        connect(ui->optInRBF, SIGNAL(stateChanged(int)), this, SLOT(coinControlUpdateLabels()));
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


        // Setup the default values
        setUpValues();

        adjustSize();
    }
}


CreateTokenDialog::~CreateTokenDialog()
{
    delete ui;
}

bool CreateTokenDialog::eventFilter(QObject *sender, QEvent *event)
{
    if (sender == ui->nameText)
    {
        if(event->type()== QEvent::FocusIn)
        {
            ui->nameText->setStyleSheet("");
        }
    }
    else if (sender == ui->tokenList)
    {
        if(event->type()== QEvent::FocusIn)
        {
            ui->tokenList->lineEdit()->setStyleSheet("");
        }
    }
    return QWidget::eventFilter(sender,event);
}

/** Helper Methods */
void CreateTokenDialog::setUpValues()
{
    ui->unitBox->setValue(0);
    ui->reissuableBox->setCheckState(Qt::CheckState::Checked);
    ui->ipfsText->hide();
    hideMessage();
    CheckFormState();
    ui->availabilityButton->setDisabled(true);

    ui->unitExampleLabel->setStyleSheet("font-weight: bold");

    // Setup the token types
    QStringList list;
    list.append(tr("Main Token") + " (" + AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), GetBurnAmount(TokenType::ROOT)) + ")");
    list.append(tr("Sub Token") + " (" + AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), GetBurnAmount(TokenType::SUB)) + ")");
    list.append(tr("Unique Token") + " (" + AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), GetBurnAmount(TokenType::UNIQUE)) + ")");
    ui->tokenType->addItems(list);
    type = IntFromTokenType(TokenType::ROOT);
    ui->tokenTypeLabel->setText(tr("Token Type") + ":");

    // Setup the token list
    ui->tokenList->hide();
    updateTokenList();

    ui->tokenFullName->setTextFormat(Qt::RichText);
    ui->tokenFullName->setStyleSheet("font-weight: bold");

    ui->tokenType->setStyleSheet("font-weight: bold");
}

void CreateTokenDialog::setupCoinControlFrame(const PlatformStyle *platformStyle)
{
    /** Update the tokencontrol frame */
    ui->frameCoinControl->setStyleSheet(QString(".QFrame {background-color: %1; padding-top: 10px; padding-right: 5px; border: none;}").arg(platformStyle->WidgetBackGroundColor().name()));
    ui->widgetCoinControl->setStyleSheet(".QWidget {background-color: transparent;}");
    /** Create the shadow effects on the frames */

    ui->labelCoinControlFeatures->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlFeatures->setFont(GUIUtil::getTopLabelFont());

    ui->labelCoinControlQuantityText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlQuantityText->setFont(GUIUtil::getSubLabelFont());

    ui->labelCoinControlAmountText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlAmountText->setFont(GUIUtil::getSubLabelFont());

    ui->labelCoinControlFeeText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlFeeText->setFont(GUIUtil::getSubLabelFont());

    ui->labelCoinControlAfterFeeText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlAfterFeeText->setFont(GUIUtil::getSubLabelFont());

    ui->labelCoinControlBytesText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlBytesText->setFont(GUIUtil::getSubLabelFont());

    ui->labelCoinControlLowOutputText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlLowOutputText->setFont(GUIUtil::getSubLabelFont());

    ui->labelCoinControlChangeText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelCoinControlChangeText->setFont(GUIUtil::getSubLabelFont());

    // Align the other labels next to the input buttons to the text in the same height
    ui->labelCoinControlAutomaticallySelected->setStyleSheet(STRING_LABEL_COLOR);

    // Align the Custom change address checkbox
    ui->checkBoxCoinControlChange->setStyleSheet(QString(".QCheckBox{ %1; }").arg(STRING_LABEL_COLOR));

}

void CreateTokenDialog::setupTokenDataView(const PlatformStyle *platformStyle)
{
    /** Update the scrollview*/

    ui->frameTokenData->setStyleSheet(QString(".QFrame {background-color: %1; padding-top: 10px; padding-right: 5px; border: none;}").arg(platformStyle->WidgetBackGroundColor().name()));

    ui->tokenTypeLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->tokenTypeLabel->setFont(GUIUtil::getSubLabelFont());

    ui->tokenNameLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->tokenNameLabel->setFont(GUIUtil::getSubLabelFont());

    ui->addressLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->addressLabel->setFont(GUIUtil::getSubLabelFont());

    ui->quantityLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->quantityLabel->setFont(GUIUtil::getSubLabelFont());

    ui->unitsLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->unitsLabel->setFont(GUIUtil::getSubLabelFont());

    ui->reissuableBox->setStyleSheet(QString(".QCheckBox{ %1; }").arg(STRING_LABEL_COLOR));
    ui->ipfsBox->setStyleSheet(QString(".QCheckBox{ %1; }").arg(STRING_LABEL_COLOR));

}

void CreateTokenDialog::setupFeeControl(const PlatformStyle *platformStyle)
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
    ui->createTokenButton->setFont(GUIUtil::getSubLabelFont());
    ui->clearButton->setFont(GUIUtil::getSubLabelFont());
    ui->labelSmartFee->setFont(GUIUtil::getSubLabelFont());
    ui->labelFeeEstimation->setFont(GUIUtil::getSubLabelFont());
    ui->labelFeeMinimized->setFont(GUIUtil::getSubLabelFont());

}

void CreateTokenDialog::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
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

void CreateTokenDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0, 0, 0, 0);
    ui->customFee->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    updateMinFeeLabel();
    updateSmartFeeLabel();
}

void CreateTokenDialog::toggleIPFSText()
{
    if (ui->ipfsBox->isChecked()) {
        ui->ipfsText->show();
    } else {
        ui->ipfsText->hide();
        ui->ipfsText->clear();
    }

    CheckFormState();
}

void CreateTokenDialog::showMessage(QString string)
{
    ui->messageLabel->setStyleSheet("color: red; font-size: 15pt;font-weight: bold;");
    ui->messageLabel->setText(string);
    ui->messageLabel->show();
}

void CreateTokenDialog::showValidMessage(QString string)
{
    ui->messageLabel->setStyleSheet("color: green; font-size: 15pt;font-weight: bold;");
    ui->messageLabel->setText(string);
    ui->messageLabel->show();
}

void CreateTokenDialog::hideMessage()
{
    ui->nameText->setStyleSheet("");
    ui->addressText->setStyleSheet("");
    if (ui->ipfsBox->isChecked())
        ui->ipfsText->setStyleSheet("");

    ui->messageLabel->hide();
}

void CreateTokenDialog::disableCreateButton()
{
    ui->createTokenButton->setDisabled(true);
}

void CreateTokenDialog::enableCreateButton()
{
    if (checkedAvailablity)
        ui->createTokenButton->setDisabled(false);
}

bool CreateTokenDialog::checkIPFSHash(QString hash)
{
    if (!hash.isEmpty()) {
        std::string error;
        if (!CheckEncodedIPFS(hash.toStdString(), error)) {
            ui->ipfsText->setStyleSheet("border: 2px solid red");
            showMessage("IPFS Hash must start with 'Qm'");
            disableCreateButton();
            return false;
        }
        else if (hash.size() != 46) {
            ui->ipfsText->setStyleSheet("border: 2px solid red");
            showMessage("IPFS Hash must have size of 46 characters");
            disableCreateButton();
            return false;
        } else if (DecodeIPFS(hash.toStdString()).empty()) {
            showMessage("IPFS hash is not valid. Please use a valid IPFS hash");
            disableCreateButton();
            return false;
        }
    }

    // No problems where found with the hash, reset the border, and hide the messages.
    hideMessage();
    ui->ipfsText->setStyleSheet("");

    return true;
}

void CreateTokenDialog::CheckFormState()
{
    disableCreateButton(); // Disable the button by default
    hideMessage();

    const CTxDestination dest = DecodeDestination(ui->addressText->text().toStdString());

    QString name = GetTokenName();

    std::string error;
    bool tokenNameValid = IsTypeCheckNameValid(TokenTypeFromInt(type), name.toStdString(), error);

    if (type != IntFromTokenType(TokenType::ROOT)) {
        if (ui->tokenList->currentText() == "")
        {
            ui->tokenList->lineEdit()->setStyleSheet(STYLE_INVALID);
            ui->availabilityButton->setDisabled(true);
            return;
        }
    }

    if (!tokenNameValid && name.size() != 0) {
        ui->nameText->setStyleSheet(STYLE_INVALID);
        showMessage(error.c_str());
        ui->availabilityButton->setDisabled(true);
        return;
    }

    if (!(ui->addressText->text().isEmpty() || IsValidDestination(dest)) && tokenNameValid) {
        ui->addressText->setStyleSheet(STYLE_INVALID);
        showMessage(tr("Warning: Invalid Alphacon address"));
        return;
    }

    if (ui->ipfsBox->isChecked())
        if (!checkIPFSHash(ui->ipfsText->text()))
            return;

    if (checkedAvailablity) {
        showValidMessage(tr("Valid Token"));
        enableCreateButton();
        ui->availabilityButton->setDisabled(true);
    } else {
        disableCreateButton();
        ui->availabilityButton->setDisabled(false);
    }
}

/** SLOTS */
void CreateTokenDialog::ipfsStateChanged()
{
    toggleIPFSText();
}

void CreateTokenDialog::checkAvailabilityClicked()
{
    QString name = GetTokenName();

    LOCK(cs_main);
    auto currentActiveTokenCache = GetCurrentTokenCache();
    if (currentActiveTokenCache) {
        CNewToken token;
        if (currentActiveTokenCache->GetTokenMetaDataIfExists(name.toStdString(), token)) {
            ui->nameText->setStyleSheet(STYLE_INVALID);
            showMessage(tr("Invalid: Token name already in use"));
            disableCreateButton();
            checkedAvailablity = false;
            return;
        } else {
            checkedAvailablity = true;
            ui->nameText->setStyleSheet(STYLE_VALID);
        }
    } else {
        checkedAvailablity = false;
        showMessage(tr("Error: Token Database not in sync"));
        disableCreateButton();
        return;
    }

    CheckFormState();
}

void CreateTokenDialog::onNameChanged(QString name)
{
    // Update the displayed name to uppercase if the type only accepts uppercase
    name = type == IntFromTokenType(TokenType::UNIQUE) ? name : name.toUpper();
    UpdateTokenNameToUpper();

    QString tokenName = name;

    // Get the identifier for the token type
    QString identifier = GetSpecialCharacter();

    if (name.size() == 0) {
        hideMessage();
        ui->availabilityButton->setDisabled(true);
        updatePresentedTokenName(name);
        return;
    }

    if (type == IntFromTokenType(TokenType::ROOT)) {
        std::string error;
        auto strName = GetTokenName();
        if (IsTypeCheckNameValid(TokenType::ROOT, strName.toStdString(), error)) {
            hideMessage();
            ui->availabilityButton->setDisabled(false);
        } else {
            ui->nameText->setStyleSheet(STYLE_INVALID);
            showMessage(tr(error.c_str()));
            ui->availabilityButton->setDisabled(true);
        }
    } else if (type == IntFromTokenType(TokenType::SUB) || type == IntFromTokenType(TokenType::UNIQUE)) {
        if (name.size() == 0) {
            hideMessage();
            ui->availabilityButton->setDisabled(true);
        }

        // If an token isn't selected. Mark the lineedit with invalid style sheet
        if (ui->tokenList->currentText() == "")
        {
            ui->tokenList->lineEdit()->setStyleSheet(STYLE_INVALID);
            ui->availabilityButton->setDisabled(true);
            return;
        }

        std::string error;
        auto tokenType = TokenTypeFromInt(type);
        auto strName = GetTokenName();
        if (IsTypeCheckNameValid(tokenType, strName.toStdString(), error)) {
            hideMessage();
            ui->availabilityButton->setDisabled(false);
        } else {
            ui->nameText->setStyleSheet(STYLE_INVALID);
            showMessage(tr(error.c_str()));
            ui->availabilityButton->setDisabled(true);
        }
    }

    // Set the tokenName
    updatePresentedTokenName(format.arg(type == IntFromTokenType(TokenType::ROOT) ? "" : ui->tokenList->currentText(), identifier, name));

    checkedAvailablity = false;
    disableCreateButton();
}

void CreateTokenDialog::onAddressNameChanged(QString address)
{
    CheckFormState();
}

void CreateTokenDialog::onIPFSHashChanged(QString hash)
{
    if (checkIPFSHash(hash))
        CheckFormState();
}

void CreateTokenDialog::onCreateTokenClicked()
{
    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

    QString name = GetTokenName();
    CAmount quantity = ui->quantitySpinBox->value() * COIN;
    int units = ui->unitBox->value();
    bool reissuable = ui->reissuableBox->isChecked();
    bool hasIPFS = ui->ipfsBox->isChecked() && !ui->ipfsText->text().isEmpty();

    std::string ipfsDecoded = "";
    if (hasIPFS)
        ipfsDecoded = DecodeIPFS(ui->ipfsText->text().toStdString());

    CNewToken token(name.toStdString(), quantity, units, reissuable ? 1 : 0, hasIPFS ? 1 : 0, ipfsDecoded);

    CWalletTx tx;
    CReserveKey reservekey(model->getWallet());
    std::pair<int, std::string> error;
    CAmount nFeeRequired;

    // Always use a CCoinControl instance, use the CoinControlDialog instance if CoinControl has been enabled
    CCoinControl ctrl;
    if (model->getOptionsModel()->getCoinControlFeatures())
        ctrl = *CoinControlDialog::coinControl;

    updateCoinControlState(ctrl);

    QString address;
    if (ui->addressText->text().isEmpty()) {
        address = model->getAddressTableModel()->addRow(AddressTableModel::Receive, "", "");
    } else {
        address = ui->addressText->text();
    }

    // Create the transaction
    if (!CreateTokenTransaction(model->getWallet(), ctrl, token, address.toStdString(), error, tx, reservekey, nFeeRequired)) {
        showMessage("Invalid: " + QString::fromStdString(error.second));
        return;
    }

    // Format confirmation message
    QStringList formatted;

    // generate bold amount string
    QString amount = "<b>" + QString::fromStdString(ValueFromAmountString(GetBurnAmount(type), 8)) + " ALP";
    amount.append("</b>");
    // generate monospace address string
    QString addressburn = "<span style='font-family: monospace;'>" + QString::fromStdString(GetBurnAddress(type));
    addressburn.append("</span>");

    QString recipientElement1;
    recipientElement1 = tr("%1 to %2").arg(amount, addressburn);
    formatted.append(recipientElement1);

    // generate the bold token amount
    QString tokenAmount = "<b>" + QString::fromStdString(ValueFromAmountString(token.nAmount, token.units)) + " " + QString::fromStdString(token.strName);
    tokenAmount.append("</b>");

    // generate the monospace address string
    QString tokenAddress = "<span style='font-family: monospace;'>" + address;
    tokenAddress.append("</span>");

    QString recipientElement2;
    recipientElement2 = tr("%1 to %2").arg(tokenAmount, tokenAddress);
    formatted.append(recipientElement2);

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

    // add total amount in all subdivision units
    questionString.append("<hr />");
    CAmount totalAmount = GetBurnAmount(type) + nFeeRequired;
    QStringList alternativeUnits;
    for (AlphaconUnits::Unit u : AlphaconUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(AlphaconUnits::formatHtmlWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1")
                                  .arg(AlphaconUnits::formatHtmlWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount)));
    questionString.append(QString("<span style='font-size:10pt;font-weight:normal;'><br />(=%2)</span>")
                                  .arg(alternativeUnits.join(" " + tr("or") + "<br />")));

    SendConfirmationDialog confirmationDialog(tr("Confirm send tokens"),
                                              questionString.arg(formatted.join("<br />")), SEND_CONFIRM_DELAY, this);
    confirmationDialog.exec();
    QMessageBox::StandardButton retval = (QMessageBox::StandardButton)confirmationDialog.result();

    if(retval != QMessageBox::Yes)
    {
        return;
    }

    // Create the transaction and broadcast it
    std::string txid;
    if (!SendTokenTransaction(model->getWallet(), tx, reservekey, error, txid)) {
        showMessage(tr("Invalid: ") + QString::fromStdString(error.second));
    } else {
        QMessageBox msgBox;
        QPushButton *copyButton = msgBox.addButton(tr("Copy"), QMessageBox::ActionRole);
        copyButton->disconnect();
        connect(copyButton, &QPushButton::clicked, this, [=](){
            QClipboard *p_Clipboard = QApplication::clipboard();
            p_Clipboard->setText(QString::fromStdString(txid), QClipboard::Mode::Clipboard);

            QMessageBox copiedBox;
            copiedBox.setText(tr("Transaction ID Copied"));
            copiedBox.exec();
        });

        QPushButton *okayButton = msgBox.addButton(QMessageBox::Ok);
        msgBox.setText(tr("Token transaction sent to network:"));
        msgBox.setInformativeText(QString::fromStdString(txid));
        msgBox.exec();

        if (msgBox.clickedButton() == okayButton) {
            clear();

            CoinControlDialog::coinControl->UnSelectAll();
            coinControlUpdateLabels();
        }
    }
}

void CreateTokenDialog::onUnitChanged(int value)
{
    QString text;
    text += "e.g. 1";
    // Add the period
    if (value > 0)
        text += ".";

    // Add the remaining zeros
    for (int i = 0; i < value; i++) {
        text += "0";
    }

    ui->unitExampleLabel->setText(text);
}

void CreateTokenDialog::onChangeAddressChanged(QString changeAddress)
{
    CheckFormState();
}

void CreateTokenDialog::onTokenTypeActivated(int index)
{
    disableCreateButton();
    checkedAvailablity = false;

    // Update the selected type
    type = index;

    // Make sure the type is only the the supported issue types
    if(!(type == IntFromTokenType(TokenType::ROOT) || type == IntFromTokenType(TokenType::SUB) || type == IntFromTokenType(TokenType::UNIQUE)))
        type = IntFromTokenType(TokenType::ROOT);

    // If the type is UNIQUE, set the units and amount to the correct value, and disable them.
    if (type == IntFromTokenType(TokenType::UNIQUE))
        setUniqueSelected();
    else
        clearSelected();

    // Get the identifier for the token type
    QString identifier = GetSpecialCharacter();

    if (index != 0) {
        ui->tokenList->show();
    } else {
        ui->tokenList->hide();
    }

    UpdateTokenNameMaxSize();

    // Set tokenName
    updatePresentedTokenName(format.arg(type == IntFromTokenType(TokenType::ROOT) ? "" : ui->tokenList->currentText(), identifier, ui->nameText->text()));

    if (ui->nameText->text().size())
        ui->availabilityButton->setDisabled(false);
    else
        ui->availabilityButton->setDisabled(true);
    ui->createTokenButton->setDisabled(true);

    // Update coinControl so it can change the amount that is being spent
    coinControlUpdateLabels();
}

void CreateTokenDialog::onTokenListActivated(int index)
{
    // Get the identifier for the token type
    QString identifier = GetSpecialCharacter();

    UpdateTokenNameMaxSize();

    // Set tokenName
    updatePresentedTokenName(format.arg(type == IntFromTokenType(TokenType::ROOT) ? "" : ui->tokenList->currentText(), identifier, ui->nameText->text()));

    if (ui->nameText->text().size())
        ui->availabilityButton->setDisabled(false);
    else
        ui->availabilityButton->setDisabled(true);
    ui->createTokenButton->setDisabled(true);
}

void CreateTokenDialog::updatePresentedTokenName(QString name)
{
    ui->tokenFullName->setText(name);
}

QString CreateTokenDialog::GetSpecialCharacter()
{
    if (type == IntFromTokenType(TokenType::SUB))
        return "/";
    else if (type == IntFromTokenType(TokenType::UNIQUE))
        return "#";

    return "";
}

QString CreateTokenDialog::GetTokenName()
{
    if (type == IntFromTokenType(TokenType::ROOT))
        return ui->nameText->text();
    else if (type == IntFromTokenType(TokenType::SUB))
        return ui->tokenList->currentText() + "/" + ui->nameText->text();
    else if (type == IntFromTokenType(TokenType::UNIQUE))
        return ui->tokenList->currentText() + "#" + ui->nameText->text();
    return "";
}

void CreateTokenDialog::UpdateTokenNameMaxSize()
{
    if (type == IntFromTokenType(TokenType::ROOT)) {
        ui->nameText->setMaxLength(30);
    } else if (type == IntFromTokenType(TokenType::SUB) || type == IntFromTokenType(TokenType::UNIQUE)) {
        ui->nameText->setMaxLength(30 - (ui->tokenList->currentText().size() + 1));
    }
}

void CreateTokenDialog::UpdateTokenNameToUpper()
{
    if (type == IntFromTokenType(TokenType::ROOT) || type == IntFromTokenType(TokenType::SUB)) {
        ui->nameText->setText(ui->nameText->text().toUpper());
    }
}

void CreateTokenDialog::updateCoinControlState(CCoinControl& ctrl)
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

void CreateTokenDialog::updateSmartFeeLabel()
{
    if(!model || !model->getOptionsModel())
        return;
    CCoinControl coin_control;
    updateCoinControlState(coin_control);
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
void CreateTokenDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void CreateTokenDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void CreateTokenDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "After fee" to clipboard
void CreateTokenDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Bytes" to clipboard
void CreateTokenDialog::coinControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text().replace(ASYMP_UTF8, ""));
}

// Coin Control: copy label "Dust" to clipboard
void CreateTokenDialog::coinControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void CreateTokenDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")).replace(ASYMP_UTF8, ""));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void CreateTokenDialog::coinControlFeatureChanged(bool checked)
{
    ui->frameCoinControl->setVisible(checked);
    ui->addressText->setVisible(checked);
    ui->addressLabel->setVisible(checked);

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();

    coinControlUpdateLabels();
}

// Coin Control: settings menu - coin control enabled/disabled by user
void CreateTokenDialog::feeControlFeatureChanged(bool checked)
{
    ui->frameFee->setVisible(checked);
}

// Coin Control: button inputs -> show actual coin control dialog
void CreateTokenDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg(platformStyle);
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void CreateTokenDialog::coinControlChangeChecked(int state)
{
    if (state == Qt::Unchecked)
    {
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->clear();
    }
    else
        // use this to re-validate an already entered address
        coinControlChangeEdited(ui->lineEditCoinControlChange->text());

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void CreateTokenDialog::coinControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel())
    {
        // Default to no change address until verified
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        const CTxDestination dest = DecodeDestination(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelCoinControlChangeLabel->setText("");
        }
        else if (!IsValidDestination(dest)) // Invalid address
        {
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid Alphacon address"));
        }
        else // Valid address
        {
            if (!model->IsSpendable(dest)) {
                ui->labelCoinControlChangeLabel->setText(tr("Warning: Unknown change address"));

                // confirmation dialog
                QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm custom change address"), tr("The address you selected for change is not part of this wallet. Any or all funds in your wallet may be sent to this address. Are you sure?"),
                                                                              QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

                if(btnRetVal == QMessageBox::Yes)
                    CoinControlDialog::coinControl->destChange = dest;
                else
                {
                    ui->lineEditCoinControlChange->setText("");
                    ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");
                    ui->labelCoinControlChangeLabel->setText("");
                }
            }
            else // Known change address
            {
                ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                if (!associatedLabel.isEmpty())
                    ui->labelCoinControlChangeLabel->setText(associatedLabel);
                else
                    ui->labelCoinControlChangeLabel->setText(tr("(no label)"));

                CoinControlDialog::coinControl->destChange = dest;
            }
        }
    }
}

// Coin Control: update labels
void CreateTokenDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel())
        return;

    updateCoinControlState(*CoinControlDialog::coinControl);

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    CoinControlDialog::fSubtractFeeFromAmount = false;

    CoinControlDialog::payAmounts.append(GetBurnAmount(type));

    if (CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}

void CreateTokenDialog::minimizeFeeSection(bool fMinimize)
{
    ui->labelFeeMinimized->setVisible(fMinimize);
    ui->buttonChooseFee  ->setVisible(fMinimize);
    ui->buttonMinimizeFee->setVisible(!fMinimize);
    ui->frameFeeSelection->setVisible(!fMinimize);
    ui->horizontalLayoutSmartFee->setContentsMargins(0, (fMinimize ? 0 : 6), 0, 0);
    fFeeMinimized = fMinimize;
}

void CreateTokenDialog::on_buttonChooseFee_clicked()
{
    minimizeFeeSection(false);
}

void CreateTokenDialog::on_buttonMinimizeFee_clicked()
{
    updateFeeMinimizedLabel();
    minimizeFeeSection(true);
}

void CreateTokenDialog::setMinimumFee()
{
    ui->customFee->setValue(GetRequiredFee(1000));
}

void CreateTokenDialog::updateFeeSectionControls()
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

void CreateTokenDialog::updateFeeMinimizedLabel()
{
    if(!model || !model->getOptionsModel())
        return;

    if (ui->radioSmartFee->isChecked())
        ui->labelFeeMinimized->setText(ui->labelSmartFee->text());
    else {
        ui->labelFeeMinimized->setText(AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), ui->customFee->value()) + "/kB");
    }
}

void CreateTokenDialog::updateMinFeeLabel()
{
    if (model && model->getOptionsModel())
        ui->checkBoxMinimumFee->setText(tr("Pay only the required fee of %1").arg(
                AlphaconUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), GetRequiredFee(1000)) + "/kB")
        );
}

void CreateTokenDialog::setUniqueSelected()
{
    ui->reissuableBox->setChecked(false);
    ui->quantitySpinBox->setValue(1);
    ui->unitBox->setValue(0);
    ui->reissuableBox->setDisabled(true);
    ui->unitBox->setDisabled(true);
    ui->quantitySpinBox->setDisabled(true);
}

void CreateTokenDialog::clearSelected()
{
    ui->reissuableBox->setDisabled(false);
    ui->unitBox->setDisabled(false);
    ui->quantitySpinBox->setDisabled(false);
    ui->reissuableBox->setChecked(true);
    ui->unitBox->setValue(0);
}

void CreateTokenDialog::updateTokenList()
{
    QStringList list;
    list << "";

    std::vector<std::string> names;
    GetAllAdministrativeTokens(model->getWallet(), names, 0);
    for (auto item : names) {
        std::string name = QString::fromStdString(item).split("!").first().toStdString();
        if (name.size() != 30)
            list << QString::fromStdString(name);
    }

    stringModel->setStringList(list);
}

void CreateTokenDialog::clear()
{
    ui->tokenType->setCurrentIndex(0);
    ui->nameText->clear();
    ui->addressText->clear();
    ui->quantitySpinBox->setValue(1);
    ui->unitBox->setValue(0);
    ui->reissuableBox->setChecked(true);
    ui->ipfsBox->setChecked(false);
    ui->ipfsText->hide();
    ui->tokenList->hide();
    ui->tokenList->setCurrentIndex(0);
    type = 0;
    ui->tokenFullName->clear();
    ui->unitBox->setDisabled(false);
    ui->quantitySpinBox->setDisabled(false);
    hideMessage();
    disableCreateButton();
}

void CreateTokenDialog::onClearButtonClicked()
{
    clear();
}

void CreateTokenDialog::focusSubToken(const QModelIndex &index)
{
    selectTypeName(1,index.data(TokenTableModel::TokenNameRole).toString());
}

void CreateTokenDialog::focusUniqueToken(const QModelIndex &index)
{
    selectTypeName(2,index.data(TokenTableModel::TokenNameRole).toString());
}

void CreateTokenDialog::selectTypeName(int type, QString name)
{
    clear();

    if (IsTokenNameAnOwner(name.toStdString()))
        name = name.left(name.size() - 1);

    ui->tokenType->setCurrentIndex(type);
    onTokenTypeActivated(type);

    ui->tokenList->setCurrentIndex(ui->tokenList->findText(name));
    onTokenListActivated(ui->tokenList->currentIndex());

    ui->nameText->setFocus();
}