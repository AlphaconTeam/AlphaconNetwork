#include <base58.h>
#include <assets/assets.h>
#include <rpc/server.h>
#include <util.h>

#include <inttypes.h>
#include <string>

using namespace std;
using namespace json_spirit;

Value issue(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "issue <to_address> <asset_name> <quantity> [units] [reissuable]\n"
            "Issue an asset with unique name.");

    std::string asset_address = params[0].get_str();
    CBitcoinAddress address(asset_address);
    if (!address.IsValid()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    std::string asset_name = params[1].get_str();
    if (!IsAssetNameValid(asset_name)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset name: " + asset_name);
    }

    int quantity = params[2].get_int();
    if (quantity < 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset quantity");
    }

    int64_t units = COIN;
    if (params[3].type() != null_type) {
        units = AmountFromValue(params[3]);
        if (!IsAssetUnitsValid(units)) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid asset units");
        }
    }

    bool reissuable = false;
    if (params[4].type() != null_type) {
        reissuable = params[4].get_bool();
    }

    std::string reissuable_str = (reissuable) ? "Reissuable" : "Not reissuable";
    
    Object result;
    result.push_back(Pair("name", asset_name));
    result.push_back(Pair("owner", asset_address));
    result.push_back(Pair("quantity", std::to_string(quantity)));
    result.push_back(Pair("units", std::to_string(units)));
    result.push_back(Pair("reissuable", reissuable_str));

    return result;
}
