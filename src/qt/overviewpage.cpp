// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "alphaconunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "platformstyle.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "tokenfilterproxy.h"
#include "tokentablemodel.h"
#include "walletmodel.h"
#include "tokenrecord.h"

#include <QAbstractItemDelegate>
#include <QPainter>
#include <validation.h>
#include <utiltime.h>

#define DECORATION_SIZE 54
#define NUM_ITEMS 5

#include <QDebug>
#include <QTimer>
#include <QScrollBar>

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    explicit TxViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
        QAbstractItemDelegate(parent), unit(AlphaconUnits::ALP),
        platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(TransactionTableModel::RawDecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);

        icon = platformStyle->SingleColorIcon(icon, COLOR_LABELS);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = platformStyle->TextColor();
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        QString amountText = index.data(TransactionTableModel::FormattedAmountRole).toString();
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }

        painter->setFont(GUIUtil::getSubLabelFont());
        // Concatenate the strings if needed before painting
        GUIUtil::concatenate(painter, address, painter->fontMetrics().width(amountText), addressRect.left(), addressRect.right());

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = platformStyle->TextColor();
        }

        painter->setPen(foreground);
        painter->drawText(addressRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        QString tokenName = index.data(TransactionTableModel::TokenNameRole).toString();

        // Concatenate the strings if needed before painting
        GUIUtil::concatenate(painter, tokenName, painter->fontMetrics().width(GUIUtil::dateTimeStr(date)), amountRect.left(), amountRect.right());

        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, tokenName);

        painter->setPen(platformStyle->TextColor());
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
    const PlatformStyle *platformStyle;

};

class TokenViewDelegate : public QAbstractItemDelegate
{
Q_OBJECT
public:
    explicit TokenViewDelegate(const PlatformStyle *_platformStyle, QObject *parent=nullptr):
            QAbstractItemDelegate(parent), unit(AlphaconUnits::ALP),
            platformStyle(_platformStyle)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        /** Get the icon for the administrator of the token */
        QPixmap pixmap = qvariant_cast<QPixmap>(index.data(Qt::DecorationRole));

        bool admin = index.data(TokenTableModel::AdministratorRole).toBool();
        bool locked = index.data(TokenTableModel::IsLockedRole).toBool();

        /** Need to know the heigh to the pixmap. If it is 0 we don't we dont own this token so dont have room for the icon */
        // int nIconSize = admin ? pixmap.height() : 0;
        int nIconSize = 25;
        int extraNameSpacing = 12;
        if (nIconSize)
            extraNameSpacing = 0;

        /** Get basic padding and half height */
        QRect mainRect = option.rect;
        int xspace = nIconSize + 25;
        int ypad = 1;

        // Create the gradient rect to draw the gradient over
        QRect gradientRect = mainRect;
        gradientRect.setTop(gradientRect.top() + 2);
        gradientRect.setBottom(gradientRect.bottom() - 2);
        gradientRect.setRight(gradientRect.right() - 20);

        int halfheight = (gradientRect.height() - 2*ypad)/2;

        /** Create the three main rectangles  (Icon, Name, Amount) */
        QRect tokenAdministratorRect(QPoint(20, gradientRect.top() + halfheight/2 - 3*ypad), QSize(nIconSize, nIconSize));
        QRect tokenNameRect(gradientRect.left() + xspace - extraNameSpacing, gradientRect.top()+ypad+(halfheight/2), gradientRect.width() - xspace, halfheight + ypad);
        QRect amountRect(gradientRect.left() + xspace, gradientRect.top()+ypad+(halfheight/2), gradientRect.width() - xspace - 16, halfheight);

        // Create the gradient for the token items
        QLinearGradient gradient(mainRect.topLeft(), mainRect.bottomRight());

        // Select the color of the gradient
        if (admin) {
            gradient.setColorAt(0, COLOR_DARK_ORANGE);
            gradient.setColorAt(1, COLOR_LIGHT_ORANGE);
        } else if (locked) {
            gradient.setColorAt(0, COLOR_DARK_GRAY);
            gradient.setColorAt(1, COLOR_LIGHT_GRAY);
        } else {
            gradient.setColorAt(0, COLOR_LIGHT_BLUE);
            gradient.setColorAt(1, COLOR_DARK_BLUE);
        }

        // Using 4 are the radius because the pixels are solid
        QPainterPath path;
        path.addRoundedRect(gradientRect, 4, 4);

        // Paint the gradient
        painter->setRenderHint(QPainter::Antialiasing);
        painter->fillPath(path, gradient);

        /** Draw token administrator icon */
        if (nIconSize)
            painter->drawPixmap(tokenAdministratorRect, pixmap);

        /** Create the font that is used for painting the token name */
        QFont nameFont;
#if !defined(Q_OS_MAC)
        nameFont.setFamily("Open Sans");
#endif
        nameFont.setPixelSize(18);
        nameFont.setWeight(QFont::Weight::Normal);
        nameFont.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, -0.4);

        /** Create the font that is used for painting the token amount */
        QFont amountFont;
#if !defined(Q_OS_MAC)
        amountFont.setFamily("Open Sans");
#endif
        amountFont.setPixelSize(14);
        amountFont.setWeight(QFont::Weight::Normal);
        amountFont.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, -0.3);

        /** Get the name and formatted amount from the data */
        QString name = index.data(TokenTableModel::TokenNameRole).toString();
        QString amountText = index.data(TokenTableModel::FormattedAmountRole).toString();

        // Setup the pens
        QColor textColor = COLOR_WHITE;

        QPen penName(textColor);

        /** Start Concatenation of Token Name */
        // Get the width in pixels that the amount takes up (because they are different font,
        // we need to do this before we call the concatenate function
        painter->setFont(amountFont);
        int amount_width = painter->fontMetrics().width(amountText);

        // Set the painter for the font used for the token name, so that the concatenate function estimated width correctly
        painter->setFont(nameFont);

        GUIUtil::concatenate(painter, name, amount_width, tokenNameRect.left(), amountRect.right());

        /** Paint the token name */
        painter->setPen(penName);
        painter->drawText(tokenNameRect, Qt::AlignLeft|Qt::AlignVCenter, name);


        /** Paint the amount */
        painter->setFont(amountFont);
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(42, 42);
    }

    int unit;
    const PlatformStyle *platformStyle;

};
#include "overviewpage.moc"
#include "alphacongui.h"
#include <QFontDatabase>

OverviewPage::OverviewPage(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentStake(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    currentWatchOnlyStake(-1),
    txdelegate(new TxViewDelegate(platformStyle, this)),
    tokendelegate(new TokenViewDelegate(platformStyle, this))
{
    ui->setupUi(this);

    // use a SingleColorIcon for the "out of sync warning" icon
    QIcon icon = platformStyle->SingleColorIcon(":/icons/warning");
    icon.addPixmap(icon.pixmap(QSize(64,64), QIcon::Normal), QIcon::Disabled); // also set the disabled icon because we are using a disabled QPushButton to work around missing HiDPI support of QLabel (https://bugreports.qt.io/browse/QTBUG-42503)
    ui->labelTransactionsStatus->setIcon(icon);
    ui->labelWalletStatus->setIcon(icon);
    ui->labelTokenStatus->setIcon(icon);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    /** Create the list of tokens */
    ui->listTokens->setItemDelegate(tokendelegate);
    ui->listTokens->setIconSize(QSize(42, 42));
    ui->listTokens->setMinimumHeight(5 * (42 + 2));
    ui->listTokens->viewport()->setAutoFillBackground(false);

    // Delay before filtering tokenes in ms
    static const int input_filter_delay = 200;

    QTimer *token_typing_delay;
    token_typing_delay = new QTimer(this);
    token_typing_delay->setSingleShot(true);
    token_typing_delay->setInterval(input_filter_delay);
    connect(ui->tokenSearch, SIGNAL(textChanged(QString)), token_typing_delay, SLOT(start()));
    connect(token_typing_delay, SIGNAL(timeout()), this, SLOT(tokenSearchChanged()));

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));
    connect(ui->listTokens, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTokenClicked(QModelIndex)));

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
    connect(ui->labelWalletStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTokenStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));
    connect(ui->labelTransactionsStatus, SIGNAL(clicked()), this, SLOT(handleOutOfSyncWarningClicks()));

    /** Set the overview page background colors, and the frames colors and padding */
    ui->tokenFrame->setStyleSheet(QString(".QFrame {background-color: %1; padding-top: 0px; padding-right: 0px;}").arg(platformStyle->WidgetBackGroundColor().name()));
    ui->frame->setStyleSheet(QString(".QFrame {background-color: %1; padding-bottom: 10px; padding-right: 0px;}").arg(platformStyle->WidgetBackGroundColor().name()));
    ui->frame_2->setStyleSheet(QString(".QFrame {background-color: %1; padding-left: 5px;}").arg(platformStyle->WidgetBackGroundColor().name()));

    /** Update the labels colors */
    ui->tokenBalanceLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->ALPBalancesLabel->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelBalanceText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelPendingText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelImmatureText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelTotalText->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelSpendable->setStyleSheet(STRING_LABEL_COLOR);
    ui->labelWatchonly->setStyleSheet(STRING_LABEL_COLOR);
    ui->recentTransactionsLabel->setStyleSheet(STRING_LABEL_COLOR);

    /** Update the labels font */
    ui->ALPBalancesLabel->setFont(GUIUtil::getTopLabelFont());
    ui->tokenBalanceLabel->setFont(GUIUtil::getTopLabelFont());
    ui->recentTransactionsLabel->setFont(GUIUtil::getTopLabelFont());

    /** Update the sub labels font */
    ui->labelBalanceText->setFont(GUIUtil::getSubLabelFont());
    ui->labelPendingText->setFont(GUIUtil::getSubLabelFont());
    ui->labelImmatureText->setFont(GUIUtil::getSubLabelFont());
    ui->labelSpendable->setFont(GUIUtil::getSubLabelFont());
    ui->labelWatchonly->setFont(GUIUtil::getSubLabelFont());
    ui->labelBalance->setFont(GUIUtil::getSubLabelFont());
    ui->labelUnconfirmed->setFont(GUIUtil::getSubLabelFont());
    ui->labelImmature->setFont(GUIUtil::getSubLabelFont());
    ui->labelWatchAvailable->setFont(GUIUtil::getSubLabelFont());
    ui->labelWatchPending->setFont(GUIUtil::getSubLabelFont());
    ui->labelWatchImmature->setFont(GUIUtil::getSubLabelFont());
    ui->labelTotalText->setFont(GUIUtil::getSubLabelFont());
    ui->labelTotal->setFont(GUIUtil::getTopLabelFontBolded());
    ui->labelWatchTotal->setFont(GUIUtil::getTopLabelFontBolded());

    /** Create the search bar for tokens */
    ui->tokenSearch->setAttribute(Qt::WA_MacShowFocusRect, 0);
    ui->tokenSearch->setStyleSheet(QString(".QLineEdit {border: 1px solid %1; border-radius: 3px;}").arg(COLOR_LABELS.name()));
    ui->tokenSearch->setAlignment(Qt::AlignVCenter);
    QFont font = ui->tokenSearch->font();
    font.setPointSize(12);
    ui->tokenSearch->setFont(font);

    QFontMetrics fm = QFontMetrics(ui->tokenSearch->font());
    ui->tokenSearch->setFixedHeight(fm.height()+ 5);

    // Trigger the call to show the tokens table if tokens are active
    showTokens();


    // context menu actions
    sendAction = new QAction(tr("Send Token"), this);
    QAction *copyAmountAction = new QAction(tr("Copy Amount"), this);
    QAction *copyNameAction = new QAction(tr("Copy Name"), this);
    issueSub = new QAction(tr("Issue Sub Token"), this);
    issueUnique = new QAction(tr("Issue Unique Token"), this);
    reissue = new QAction(tr("Reissue Token"), this);


    sendAction->setObjectName("Send");
    issueSub->setObjectName("Sub");
    issueUnique->setObjectName("Unique");
    reissue->setObjectName("Reissue");
    copyNameAction->setObjectName("Copy Name");
    copyAmountAction->setObjectName("Copy Amount");

    // context menu
    contextMenu = new QMenu(this);
    contextMenu->addAction(sendAction);
    contextMenu->addAction(issueSub);
    contextMenu->addAction(issueUnique);
    contextMenu->addAction(reissue);
    contextMenu->addSeparator();
    contextMenu->addAction(copyNameAction);
    contextMenu->addAction(copyAmountAction);
    // context menu signals
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        Q_EMIT transactionClicked(filter->mapToSource(index));
}

void OverviewPage::handleTokenClicked(const QModelIndex &index)
{
    if(tokenFilter) {


        QString name = index.data(TokenTableModel::TokenNameRole).toString();
        bool fOwner = false;
        if (IsTokenNameAnOwner(name.toStdString())) {
            fOwner = true;
            name = name.left(name.size() - 1);
            sendAction->setDisabled(true);
        } else {
            sendAction->setDisabled(false);
        }

        if (!index.data(TokenTableModel::AdministratorRole).toBool()) {
            issueSub->setDisabled(true);
            issueUnique->setDisabled(true);
            reissue->setDisabled(true);
        } else {
            issueSub->setDisabled(false);
            issueUnique->setDisabled(false);
            reissue->setDisabled(true);
            CNewToken token;
            auto currentActiveTokenCache = GetCurrentTokenCache();
            if (currentActiveTokenCache && currentActiveTokenCache->GetTokenMetaDataIfExists(name.toStdString(), token))
                if (token.nReissuable)
                    reissue->setDisabled(false);

        }

        QAction* action = contextMenu->exec(QCursor::pos());

        if (action) {
            if (action->objectName() == "Send")
                    Q_EMIT tokenSendClicked(tokenFilter->mapToSource(index));
            else if (action->objectName() == "Sub")
                    Q_EMIT tokenIssueSubClicked(tokenFilter->mapToSource(index));
            else if (action->objectName() == "Unique")
                    Q_EMIT tokenIssueUniqueClicked(tokenFilter->mapToSource(index));
            else if (action->objectName() == "Reissue")
                    Q_EMIT tokenReissueClicked(tokenFilter->mapToSource(index));
            else if (action->objectName() == "Copy Name")
                GUIUtil::setClipboard(index.data(TokenTableModel::TokenNameRole).toString());
            else if (action->objectName() == "Copy Amount")
                GUIUtil::setClipboard(index.data(TokenTableModel::FormattedAmountRole).toString());
        }
    }

}

void OverviewPage::handleOutOfSyncWarningClicks()
{
    Q_EMIT outOfSyncWarningClicked();
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& stake, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance, const CAmount& watchOnlyStake)
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentStake = stake;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
    currentWatchOnlyStake = watchOnlyStake;
    ui->labelBalance->setText(AlphaconUnits::formatWithUnit(unit, balance, false, AlphaconUnits::separatorAlways));
    ui->labelUnconfirmed->setText(AlphaconUnits::formatWithUnit(unit, unconfirmedBalance, false, AlphaconUnits::separatorAlways));
    ui->labelImmature->setText(AlphaconUnits::formatWithUnit(unit, immatureBalance, false, AlphaconUnits::separatorAlways));
    ui->labelStake->setText(AlphaconUnits::formatWithUnit(unit, stake, false, AlphaconUnits::separatorAlways));
    ui->labelTotal->setText(AlphaconUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, AlphaconUnits::separatorAlways));
    ui->labelWatchAvailable->setText(AlphaconUnits::formatWithUnit(unit, watchOnlyBalance, false, AlphaconUnits::separatorAlways));
    ui->labelWatchPending->setText(AlphaconUnits::formatWithUnit(unit, watchUnconfBalance, false, AlphaconUnits::separatorAlways));
    ui->labelWatchImmature->setText(AlphaconUnits::formatWithUnit(unit, watchImmatureBalance, false, AlphaconUnits::separatorAlways));
    ui->labelWatchTotal->setText(AlphaconUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance + watchOnlyStake, false, AlphaconUnits::separatorAlways));

    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showStake = stake != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;
    bool showWatchOnlyStake = watchOnlyStake != 0;

    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance
    ui->labelStake->setVisible(showStake || showWatchOnlyStake);
    ui->labelStakeText->setVisible(showStake || showWatchOnlyStake);
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly) {
        ui->labelWatchImmature->hide();
    }
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter.reset(new TransactionFilterProxy());
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter.get());
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        tokenFilter.reset(new TokenFilterProxy());
        tokenFilter->setSourceModel(model->getTokenTableModel());
        tokenFilter->sort(TokenTableModel::TokenNameRole, Qt::DescendingOrder);
        ui->listTokens->setModel(tokenFilter.get());
        ui->listTokens->setAutoFillBackground(false);

        ui->tokenVerticalSpaceWidget->setStyleSheet("background-color: transparent");
        ui->tokenVerticalSpaceWidget2->setStyleSheet("background-color: transparent");


        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(), model->getStake(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance(), model->getWatchStake());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("ALP")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance, currentStake,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance, currentWatchOnlyStake);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString &warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
    if (AreTokensDeployed()) {
        ui->labelTokenStatus->setVisible(fShow);
    }
}

void OverviewPage::showTokens()
{
    if (AreTokensDeployed()) {
        ui->tokenFrame->show();
        ui->tokenBalanceLabel->show();
        ui->labelTokenStatus->show();

        // Disable the vertical space so that listTokens goes to the bottom of the screen
        ui->tokenVerticalSpaceWidget->hide();
        ui->tokenVerticalSpaceWidget2->hide();
    } else {
        ui->tokenFrame->hide();
        ui->tokenBalanceLabel->hide();
        ui->labelTokenStatus->hide();

        // This keeps the ALP balance grid from expanding and looking terrible when token balance is hidden
        ui->tokenVerticalSpaceWidget->show();
        ui->tokenVerticalSpaceWidget2->show();
    }
}

void OverviewPage::tokenSearchChanged()
{
    if (!tokenFilter)
        return;
    tokenFilter->setTokenNamePrefix(ui->tokenSearch->text());
}
