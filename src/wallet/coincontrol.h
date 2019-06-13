// Copyright (c) 2011-2016 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2019 The Alphacon Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ALPHACON_WALLET_COINCONTROL_H
#define ALPHACON_WALLET_COINCONTROL_H

#include "policy/feerate.h"
#include "policy/fees.h"
#include "primitives/transaction.h"
#include "wallet/wallet.h"

#include <boost/optional.hpp>

/** Coin Control Features. */
class CCoinControl
{
public:
    CTxDestination destChange;
    //! If false, allows unselected inputs, but requires all selected inputs be used
    bool fAllowOtherInputs;
    //! Includes watch only addresses which match the ISMINE_WATCH_SOLVABLE criteria
    bool fAllowWatchOnly;
    //! Override automatic min/max checks on fee, m_feerate must be set if true
    bool fOverrideFeeRate;
    //! Override the default payTxFee if set
    boost::optional<CFeeRate> m_feerate;
    //! Override the default confirmation target if set
    boost::optional<unsigned int> m_confirm_target;
    //! Signal BIP-125 replace by fee.
    bool signalRbf;
    //! Fee estimation mode to control arguments to estimateSmartFee
    FeeEstimateMode m_fee_mode;

    /** TOKENS START */
    //! Name of the token that is selected, used when sending tokens with coincontrol
    std::string strTokenSelected;
    /** TOKENS END */

    CCoinControl()
    {
        SetNull();
    }

    void SetNull()
    {
        destChange = CNoDestination();
        fAllowOtherInputs = false;
        fAllowWatchOnly = false;
        setSelected.clear();
        m_feerate.reset();
        fOverrideFeeRate = false;
        m_confirm_target.reset();
        signalRbf = fWalletRbf;
        m_fee_mode = FeeEstimateMode::UNSET;
        strTokenSelected = "";
        setTokensSelected.clear();
    }

    bool HasSelected() const
    {
        return (setSelected.size() > 0);
    }

    bool HasTokenSelected() const
    {
        return (setTokensSelected.size() > 0);
    }

    bool IsSelected(const COutPoint& output) const
    {
        return (setSelected.count(output) > 0);
    }

    bool IsTokenSelected(const COutPoint& output) const
    {
        return (setTokensSelected.count(output) > 0);
    }

    void Select(const COutPoint& output)
    {
        setSelected.insert(output);
    }

    void SelectToken(const COutPoint& output)
    {
        setTokensSelected.insert(output);
    }


    void UnSelect(const COutPoint& output)
    {
        setSelected.erase(output);
        if (!setSelected.size())
            strTokenSelected = "";
    }

    void UnSelectToken(const COutPoint& output)
    {
        setTokensSelected.erase(output);
        if (!setSelected.size())
            strTokenSelected = "";
    }

    void UnSelectAll()
    {
        setSelected.clear();
        strTokenSelected = "";
        setTokensSelected.clear();
    }

    void ListSelected(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setSelected.begin(), setSelected.end());
    }

    void ListSelectedTokens(std::vector<COutPoint>& vOutpoints) const
    {
        vOutpoints.assign(setTokensSelected.begin(), setTokensSelected.end());
    }

private:
    std::set<COutPoint> setSelected;
    std::set<COutPoint> setTokensSelected;
};

#endif // ALPHACON_WALLET_COINCONTROL_H
