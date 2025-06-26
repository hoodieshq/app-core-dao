#include "display.h"

#include "../bitcoin_app_base/src/ui/display.h"
#include "../bitcoin_app_base/src/ui/menu.h"
#include "io.h"
#include "core.h"
#include "nbgl_use_case.h"
#include "time_helper.h"

#define MAX_N_PAIRS 9

static void review_choice(bool approved) {
    set_ux_flow_response(approved);  // sets the return value of io_ui_process

    if (approved) {
        // nothing to do in this case; after signing, the responsibility to show the main menu
        // goes back to the base app's handler
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, ui_menu_main);
    }
}

#ifdef SCREEN_SIZE_WALLET

static int review_title(tx_type_t type, char *out_buf, size_t out_buf_len) {
    const char *review_title = NULL;

    if (type & TYPE_TX_LOCK) {
        review_title = "Review transaction to lock BTC to earn CORE";
    } else {
        review_title = "Review transaction to unlock BTC";
    }

    return snprintf(out_buf, out_buf_len, "%s", review_title);
}

static int sign_title(tx_type_t type, char *out_buf, size_t out_buf_len) {
    const char *review_title = NULL;

    if (type & TYPE_TX_LOCK) {
        review_title = "Sign transaction to lock BTC to earn CORE?";
    } else {
        review_title = "Sign transaction to unlock BTC?";
    }

    return snprintf(out_buf, out_buf_len, "%s", review_title);
}

#else

static const char *tx_type_to_str(tx_type_t type) {
    if (type & TYPE_TX_LOCK) {
        return "lock";
    } else {
        return "unlock";
    }
}

static int review_title(tx_type_t type, char *out_buf, size_t out_buf_len) {
    const char *operation_txt = tx_type_to_str(type);

    return snprintf(out_buf, out_buf_len, "Review transaction to %s BTC", operation_txt);
}

static int sign_title(tx_type_t type, char *out_buf, size_t out_buf_len) {
    (void) type;

    return snprintf(out_buf, out_buf_len, "Sign transaction");
}

#endif

bool display_transaction(dispatcher_context_t *dc,
                         int64_t value_spent,
                         uint64_t fee,
                         core_dao_tx_info_t *info) {
    nbgl_layoutTagValue_t pairs[MAX_N_PAIRS];
    nbgl_layoutTagValueList_t pairList;

    // Format values
    char *chain_id;

    char review_title_str[46] = {0};
    char sign_title_str[46] = {0};
    char value_str[32] = {0};
    char fee_str[32] = {0};
    char unstake_value_str[32] = {0};
    char delegator_str[43] = {0};
    char validator_str[43] = {0};
    char locktime_str[DATETIME_STR_LEN] = {0};
    char core_fee_str[10] = {0};
    uint64_t value_spent_abs = value_spent < 0 ? -value_spent : value_spent;
    format_sats_amount(COIN_COINID_SHORT, value_spent_abs, value_str);
    format_sats_amount(COIN_COINID_SHORT, fee, fee_str);
    format_sats_amount(COIN_COINID_SHORT, info->unlock_amount, unstake_value_str);
    format_address(info->delegator, 20, delegator_str, sizeof(delegator_str));
    format_address(info->validator, 20, validator_str, sizeof(validator_str));
    timestamp_to_string(info->locktime, locktime_str);
    snprintf(core_fee_str, sizeof(core_fee_str), "%d CORE", info->fee);

    if (info->chain_id == CHAID_ID_MAINNET) {
        chain_id = (char *) "Mainnet";
    } else if (info->chain_id == CHAIN_ID_TESTNET) {
        chain_id = (char *) "Testnet";
    } else if (info->chain_id == CHAIN_ID_TESTNET2) {
        chain_id = (char *) "Testnet2";
    } else {
        // Fallback to compile time defined chain ID
#ifdef CORE_DAO_MAINNET
        chain_id = (char *) "Mainnet";
        info->chain_id = CHAID_ID_MAINNET;
#else
        chain_id = (char *) "Testnet";
        info->chain_id = CHAIN_ID_TESTNET;
#endif
    }

    review_title(info->type, review_title_str, sizeof(review_title_str));
    sign_title(info->type, sign_title_str, sizeof(sign_title_str));

    int n_pairs = 0;

    if (info->type & TYPE_TX_LOCK) {
        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Lock amount",
            .value = value_str,
        };

        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Lock until (UTC)",
            .value = locktime_str,
        };
    }

    if (info->type == TYPE_TX_UNLOCK) {
        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Unlock amount",
            .value = unstake_value_str,
        };
    }

    if (info->type & TYPE_TX_LOCK) {
        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Delegator",
            .value = delegator_str,
        };

        pairs[n_pairs++] = (nbgl_layoutTagValue_t){.item = "Validator",
                                                   .value = validator_str,
                                                   .forcePageStart = true};

        pairs[n_pairs++] = (nbgl_layoutTagValue_t){.item = "CORE fee", .value = core_fee_str};
    }

    pairs[n_pairs++] = (nbgl_layoutTagValue_t){
        .item = "Fees",
        .value = fee_str,
    };

    if (info->chain_id != CHAID_ID_MAINNET) {
        pairs[n_pairs++] = (nbgl_layoutTagValue_t){.item = "Network",
                                                   .value = chain_id,
                                                   .forcePageStart = TYPE_TX_UNLOCK != info->type};
    }

    assert(n_pairs <= MAX_N_PAIRS);

    // Setup list
    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = n_pairs;
    pairList.pairs = pairs;

    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &ICON_APP_HOME,
                       review_title_str,
                       NULL,
                       sign_title_str,
                       review_choice);

    // blocking call until the user approves or rejects the transaction
    bool result = io_ui_process(dc);
    if (!result) {
        SEND_SW(dc, SW_DENY);
        return false;
    }

    return true;
}
