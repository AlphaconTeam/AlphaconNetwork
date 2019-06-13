// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "sendtokensentry.h"
#include "ui_sendtokensentry.h"
//#include "sendcoinsentry.h"
//#include "ui_sendcoinsentry.h"

#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "walletmodel.h"
#include "tokencontroldialog.h"
#include "guiconstants.h"

#include "wallet/coincontrol.h"

#include <QApplication>
#include <QClipboard>
#include <validation.h>
#include <core_io.h>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QCompleter>

SendTokensEntry::SendTokensEntry(const PlatformStyle *_platformStyle, const QStringList myTokensNames, QWidget *parent) :
    QStackedWidget(parent),
    ui(new Ui::SendTokensEntry),
    model(0),
    platformStyle(_platformStyle)
{
    ui->setupUi(this);

    ui->addressBookButton->setIcon(platformStyle->SingleColorIcon(":/icons/address-book"));
    ui->pasteButton->setIcon(platformStyle->SingleColorIcon(":/icons/editpaste"));
    ui->deleteButton->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_is->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));
    ui->deleteButton_s->setIcon(platformStyle->SingleColorIcon(":/icons/remove"));

    setCurrentWidget(ui->SendCoins);

    if (platformStyle->getUseExtraSpacing())
        ui->payToLayout->setSpacing(4);
#if QT_VERSION >= 0x040700
    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));
    ui->addAsLabelLockTime->setPlaceholderText(tr("Enter height or timestamp lock time for token transfer (default = 0)"));
#endif

    // normal alphacon address field
    GUIUtil::setupAddressWidget(ui->payTo, this);
    // just a label for displaying alphacon address(es)
    ui->payTo_is->setFont(GUIUtil::fixedPitchFont());

    // Connect signals
    connect(ui->payTokenAmount, SIGNAL(valueChanged()), this, SIGNAL(payAmountChanged()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_is, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_s, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->tokenSelectionBox, SIGNAL(activated(int)), this, SLOT(onTokenSelected(int)));
    connect(ui->administratorCheckbox, SIGNAL(clicked()), this, SLOT(onSendOwnershipChanged()));

    if (!gArgs.GetBoolArg("-advancedui", false)) {
        ui->administratorCheckbox->hide();
    }

    ui->administratorCheckbox->setToolTip(tr("Select to view administrator tokens to transfer"));

    /** Setup the token list combobox */
    stringModel = new QStringListModel;
    stringModel->insertRow(stringModel->rowCount());
    stringModel->setData(stringModel->index(stringModel->rowCount() - 1, 0), "", Qt::DisplayRole);

    for (auto name : myTokensNames)
    {
        stringModel->insertRow(stringModel->rowCount());
        stringModel->setData(stringModel->index(stringModel->rowCount() - 1, 0), name, Qt::DisplayRole);
    }

    proxy = new QSortFilterProxyModel;
    proxy->setSourceModel(stringModel);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);

    ui->tokenSelectionBox->setModel(proxy);
    ui->tokenSelectionBox->setEditable(true);

    completer = new QCompleter(proxy,this);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    ui->tokenSelectionBox->setCompleter(completer);

    ui->tokenSelectionBox->lineEdit()->setPlaceholderText(tr("Select an token to transfer"));
    ui->tokenSelectionBox->setMinimumWidth(32);

    /** Setup the amount box */
    ui->ownershipWarningMessage->hide();

    fShowAdministratorList = false;

    this->setStyleSheet(QString(".SendTokensEntry {background-color: %1; padding-top: 10px; padding-right: 30px; border: none;}").arg(platformStyle->SendEntriesBackGroundColor().name()));

    ui->tokenBoxLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->tokenBoxLabel->setFont(GUIUtil::getSubLabelFont());

    ui->payToLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->payToLabel->setFont(GUIUtil::getSubLabelFont());

    ui->labellLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->labellLabel->setFont(GUIUtil::getSubLabelFont());

    ui->tokenLockTimeLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->tokenLockTimeLabel->setFont(GUIUtil::getSubLabelFont());

    ui->amountLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->amountLabel->setFont(GUIUtil::getSubLabelFont());

    ui->messageLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->messageLabel->setFont(GUIUtil::getSubLabelFont());

    ui->payTokenAmount->setUnit(MAX_UNIT);
    ui->payTokenAmount->setDisabled(false);

    ui->administratorCheckbox->setStyleSheet(QString(".QCheckBox{ %1; }").arg(STRING_LABEL_COLOR));

    ui->tokenSelectionBox->setFont(GUIUtil::getSubLabelFont());
    ui->administratorCheckbox->setFont(GUIUtil::getSubLabelFont());
    ui->payTo->setFont(GUIUtil::getSubLabelFont());
    ui->addAsLabel->setFont(GUIUtil::getSubLabelFont());
    ui->addAsLabelLockTime->setFont(GUIUtil::getSubLabelFont());
    ui->payTokenAmount->setFont(GUIUtil::getSubLabelFont());
    ui->messageTextLabel->setFont(GUIUtil::getSubLabelFont());
    ui->tokenAmountLabel->setFont(GUIUtil::getSubLabelFont());
    ui->ownershipWarningMessage->setFont(GUIUtil::getSubLabelFont());
}

SendTokensEntry::~SendTokensEntry()
{
    delete ui;
}

void SendTokensEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendTokensEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(platformStyle, AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payTokenAmount->setFocus();
    }
}

void SendTokensEntry::on_payTo_textChanged(const QString &address)
{
    updateLabel(address);
}

void SendTokensEntry::setModel(WalletModel *_model)
{
    this->model = _model;

//    if (_model && _model->getOptionsModel())
//        connect(_model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    clear();
}

void SendTokensEntry::clear()
{
    // clear UI elements for normal payment
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->addAsLabelLockTime->clear();
    ui->messageTextLabel->clear();
    ui->messageTextLabel->hide();
    ui->messageLabel->hide();
    // clear UI elements for unauthenticated payment request
    ui->memoTextLabel_is->clear();
    // clear UI elements for authenticated payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();

    ui->payTokenAmount->clear();

    // Reset the selected token
    ui->tokenSelectionBox->setCurrentIndex(0);
}

void SendTokensEntry::deleteClicked()
{
    Q_EMIT removeEntry(this);
}

bool SendTokensEntry::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    if (!model->validateAddress(ui->payTo->text()))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    if (ui->tokenSelectionBox->currentIndex() == 0) {
        ui->tokenSelectionBox->lineEdit()->setStyleSheet(STYLE_INVALID);
        retval = false;
    }

    if (!ui->payTokenAmount->validate())
    {
        retval = false;
    }

    if (ui->payTokenAmount->value(0) <= 0)
    {
        ui->payTokenAmount->setValid(false);
        retval = false;
    }

    if (ui->addAsLabelLockTime->text() != "") {
        QRegExp re("\\d*");
        if (!re.exactMatch(ui->addAsLabelLockTime->text())) {
            ui->addAsLabelLockTime->setValid(false);
            retval = false;
        }
    }

    // TODO check to make sure the payAmount value is within the constraints of how much you own

    return retval;
}

SendTokensRecipient SendTokensEntry::getValue()
{
    // Payment request
    if (recipient.paymentRequest.IsInitialized())
        return recipient;

    // Normal payment
    recipient.tokenName = ui->tokenSelectionBox->currentText();
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    if (ui->addAsLabelLockTime->text() != "") {
        recipient.tokenLockTime = ui->addAsLabelLockTime->text().toInt();
    } else {
        recipient.tokenLockTime = 0;
    }
    recipient.amount = ui->payTokenAmount->value();
    recipient.message = ui->messageTextLabel->text();

    return recipient;
}

QWidget *SendTokensEntry::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, ui->payTo);
    QWidget::setTabOrder(ui->payTo, ui->addAsLabel);
    QWidget::setTabOrder(ui->addressBookButton, ui->pasteButton);
    QWidget::setTabOrder(ui->pasteButton, ui->deleteButton);
    return ui->deleteButton;
}

void SendTokensEntry::setValue(const SendTokensRecipient &value)
{
    recipient = value;

    if (recipient.tokenName != "") {
        int index = ui->tokenSelectionBox->findText(recipient.tokenName);
        ui->tokenSelectionBox->setCurrentIndex(index);
        onTokenSelected(index);
    }
}

void SendTokensEntry::setAddress(const QString &address)
{
    ui->payTo->setText(address);
    ui->payTokenAmount->setFocus();
}

bool SendTokensEntry::isClear()
{
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() && ui->payTo_s->text().isEmpty();
}

void SendTokensEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendTokensEntry::setFocusTokenListBox()
{
    ui->tokenSelectionBox->setFocus();
}

bool SendTokensEntry::updateLabel(const QString &address)
{
    if(!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}

void SendTokensEntry::onTokenSelected(int index)
{
    ui->tokenSelectionBox->lineEdit()->setStyleSheet("");
    QString name = ui->tokenSelectionBox->currentText();
    // If the name
    if (index == 0) {
        ui->tokenAmountLabel->clear();
        if(!ui->administratorCheckbox->isChecked()) {
            ui->payTokenAmount->setDisabled(false);
            
        }
        ui->payTokenAmount->clear();
        return;
    }

    // Check to see if the token selected is an ownership token
    bool fIsOwnerToken = false;
    if (IsTokenNameAnOwner(name.toStdString())) {
        fIsOwnerToken = true;
        name = name.split("!").first();
    }

    LOCK(cs_main);
    auto currentActiveTokenCache = GetCurrentTokenCache();
    CNewToken token;

    // Get the token metadata if it exists. This isn't called on the administrator token because that doesn't have metadata
    if (!currentActiveTokenCache->GetTokenMetaDataIfExists(name.toStdString(), token)) {
        // This should only happen if the user, selected an token that was issued from tokencontrol and tries to transfer it before it is mined.
        clear();
        ui->messageLabel->show();
        ui->messageTextLabel->show();
        ui->messageTextLabel->setText(tr("Failed to get token metadata for: ") + name + "." + tr(" The transaction in which the token was issued must be mined into a block before you can transfer it"));
        ui->tokenAmountLabel->clear();
        return;
    }

    CAmount amount = 0;

    if(!model || !model->getWallet())
        return;

    std::map<std::string, std::vector<COutput> > mapTokens;
    model->getWallet()->AvailableTokens(mapTokens, true, TokenControlDialog::tokenControl);

    // Add back the OWNER_TAG (!) that was removed above
    if (fIsOwnerToken)
        name = name + OWNER_TAG;


    if (!mapTokens.count(name.toStdString())) {
        clear();
        ui->messageLabel->show();
        ui->messageTextLabel->show();
        ui->messageTextLabel->setText(tr("Failed to get token outpoints from database"));
        return;
    }

    auto vec = mapTokens.at(name.toStdString());

    // Go through all of the mapTokens to get the total count of tokens
    for (auto txout : vec) {
        CTokenOutputEntry data;
        if (GetTokenData(txout.tx->tx->vout[txout.i].scriptPubKey, data))
            amount += data.nAmount;
    }

    int units = fIsOwnerToken ? OWNER_UNITS : token.units;

    QString displayBalance = TokenControlDialog::tokenControl->HasTokenSelected() ? tr("Selected Balance") : tr("Wallet Balance");

    ui->tokenAmountLabel->setText(
            displayBalance + ": <b>" + QString::fromStdString(ValueFromAmountString(amount, units)) + "</b> " + name);

    ui->messageLabel->hide();
    ui->messageTextLabel->hide();

    // If it is an ownership token lock the amount
    if (!fIsOwnerToken) {
        ui->payTokenAmount->setUnit(token.units);
        ui->payTokenAmount->setDisabled(false);
    }
}

void SendTokensEntry::onSendOwnershipChanged()
{
    switchAdministratorList(true);
}

void SendTokensEntry::CheckOwnerBox() {
    fUsingTokenControl = true;
    switchAdministratorList();
}

void SendTokensEntry::IsTokenControl(bool fIsTokenControl, bool fIsOwner)
{
    if (fIsOwner) {
        CheckOwnerBox();
    }
    if (fIsTokenControl) {
        ui->administratorCheckbox->setDisabled(true);
        fUsingTokenControl = true;
    }
}

void SendTokensEntry::setCurrentIndex(int index)
{
    if (index < ui->tokenSelectionBox->count()) {
        ui->tokenSelectionBox->setCurrentIndex(index);
        ui->tokenSelectionBox->activated(index);
    }
}

void SendTokensEntry::refreshTokenList()
{
    switchAdministratorList(false);
}

void SendTokensEntry::switchAdministratorList(bool fSwitchStatus)
{
    if(!model)
        return;

    if (fSwitchStatus)
        fShowAdministratorList = !fShowAdministratorList;

    if (fShowAdministratorList) {
        ui->administratorCheckbox->setChecked(true);
        if (!TokenControlDialog::tokenControl->HasTokenSelected()) {
            std::vector<std::string> names;
            GetAllAdministrativeTokens(model->getWallet(), names, 0);

            QStringList list;
            list << "";
            for (auto name: names)
                list << QString::fromStdString(name);

            stringModel->setStringList(list);

            ui->tokenSelectionBox->lineEdit()->setPlaceholderText(tr("Select an administrator token to transfer"));
            ui->tokenSelectionBox->setFocus();
        } else {
            ui->payTo->setFocus();
        }

        ui->payTokenAmount->setUnit(MIN_UNIT); // Min unit because this is an administrator token
        ui->payTokenAmount->setValue(1); // When using TokenAmountField, you must use 1 instead of 1 * COIN, because of the way that TokenAmountField uses the unit and value to display the amount
        ui->payTokenAmount->setDisabled(true);

        ui->addAsLabelLockTime->setDisabled(true);
        ui->addAsLabelLockTime->clear();

        ui->tokenAmountLabel->clear();

        ui->ownershipWarningMessage->setText(tr("Warning: Transferring administrator token"));
        ui->ownershipWarningMessage->setStyleSheet("color: red");
        ui->ownershipWarningMessage->show();
    } else {
        ui->administratorCheckbox->setChecked(false);
        if (!TokenControlDialog::tokenControl->HasTokenSelected()) {
            std::vector<std::string> names;
            GetAllMyTokens(model->getWallet(), names, 0);
            QStringList list;
            list << "";
            for (auto name : names) {
                if (!IsTokenNameAnOwner(name))
                    list << QString::fromStdString(name);
            }

            stringModel->setStringList(list);
            ui->tokenSelectionBox->lineEdit()->setPlaceholderText(tr("Select an token to transfer"));
            ui->payTokenAmount->clear();
            ui->payTokenAmount->setUnit(MAX_UNIT);
            ui->payTokenAmount->setDisabled(false);
            ui->tokenAmountLabel->clear();

            ui->addAsLabelLockTime->setDisabled(false);
            ui->addAsLabelLockTime->clear();

            ui->tokenSelectionBox->setFocus();
        } else {
            ui->payTo->setFocus();
        }
        ui->ownershipWarningMessage->hide();
    }
}
