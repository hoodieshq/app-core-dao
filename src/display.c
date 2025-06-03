#ifdef HAVE_NBGL

#include "display.h"

#include "../bitcoin_app_base/src/ui/display.h"
#include "../bitcoin_app_base/src/ui/menu.h"
#include "io.h"
#include "core.h"
#include "nbgl_use_case.h"
#include "time_helper.h"

static void review_choice(bool approved) {
    set_ux_flow_response(approved);  // sets the return value of io_ui_process

    if (approved) {
        // nothing to do in this case; after signing, the responsibility to show the main menu
        // goes back to the base app's handler
    } else {
        nbgl_useCaseReviewStatus(STATUS_TYPE_TRANSACTION_REJECTED, ui_menu_main);
    }
}

#define MAX_N_PAIRS 9

bool display_transaction(
    dispatcher_context_t *dc,
    int64_t value_spent, 
    uint64_t fee, 
    core_dao_tx_info_t *info
    ) {
    nbgl_layoutTagValue_t pairs[MAX_N_PAIRS];
    nbgl_layoutTagValueList_t pairList;

    // Format values
    char *chain_id;
    char *operation_type = NULL;
    char value_str[32] = { 0 };
    char fee_str[32] = { 0 };
    char unstake_value_str[32] = { 0 };
    char delegator_str[41] = { 0 };
    char validator_str[41] = { 0 };
    char locktime_str[DATETIME_STR_LEN] = { 0 };
    char core_fee_str[4] = { 0 };
    uint64_t value_spent_abs = value_spent < 0 ? -value_spent : value_spent;
    format_sats_amount(COIN_COINID_SHORT, value_spent_abs, value_str);
    format_sats_amount(COIN_COINID_SHORT, fee, fee_str);
    format_sats_amount(COIN_COINID_SHORT, info->unlock_amount, unstake_value_str);
    buffer_to_hex(info->delegator, 20, delegator_str, 41);
    buffer_to_hex(info->validator, 20, validator_str, 41);
    timestamp_to_string(info->locktime, locktime_str);
    snprintf(core_fee_str, 4, "%d", info->fee);

    if (info->chain_id == CHAID_ID_MAINNET) {
        chain_id = (char *)"Mainnet";
    } else if (info->chain_id == CHAIN_ID_TESTNET) {
        chain_id = (char *)"Testnet";
    } else if (info->chain_id == CHAIN_ID_TESTNET2) {
        chain_id = (char *)"Testnet2";
    } else {
        chain_id = (char *)"Unknown";
    }

    if (info->type & TYPE_TX_LOCK && info->type & TYPE_TX_UNLOCK) {
        operation_type = (char *)"Restake";
    } else if (info->type & TYPE_TX_LOCK) {
        operation_type = (char *)"Stake";
    } else if (info->type & TYPE_TX_UNLOCK) {
        operation_type = (char *)"Unstake";
    }

    int n_pairs = 0;
    pairs[n_pairs++] = (nbgl_layoutTagValue_t){
        .item = "Transaction type",
        .value = operation_type,
    };

    if (info->type & TYPE_TX_LOCK) {
        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Stake amount",
            .value = value_str,
        };
    }

    if (info->type & TYPE_TX_UNLOCK) {
        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Unstake amount",
            .value = unstake_value_str,
        };
    }

    if (info->type & TYPE_TX_LOCK) {
        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Delegator",
            .value = delegator_str,
        };

        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Validator",
            .value = validator_str,
            .forcePageStart = true
        };

        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Network",
            .value = chain_id,
        };

        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Locktime (UTC+0)",
            .value = locktime_str,
            .forcePageStart = true
        };

        pairs[n_pairs++] = (nbgl_layoutTagValue_t){
            .item = "Core fee",
            .value = core_fee_str
        };
    }

    pairs[n_pairs++] = (nbgl_layoutTagValue_t){
        .item = "Fee",
        .value = fee_str,
    };
    

    assert(n_pairs <= MAX_N_PAIRS);

    // Setup list
    pairList.nbMaxLinesForValue = 0;
    pairList.nbPairs = n_pairs;
    pairList.pairs = pairs;

    nbgl_useCaseReview(TYPE_TRANSACTION,
                       &pairList,
                       &C_App_64px,
                       "Review CoreDAO\ntransaction",
                       NULL,
                       "Sign CoreDAO\ntransaction?",
                       review_choice);

    // blocking call until the user approves or rejects the transaction
    bool result = io_ui_process(dc);
    if (!result) {
        SEND_SW(dc, SW_DENY);
        return false;
    }

    return true;
}

#endif // HAVE_NBGL
