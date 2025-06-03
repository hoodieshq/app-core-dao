#ifdef HAVE_BAGL

#include "display.h"

#include "../bitcoin_app_base/src/ui/display.h"
#include "../bitcoin_app_base/src/ui/menu.h"
#include "io.h"
#include "core.h"
#include "time_helper.h"

#include "os.h"
#include "ux.h"
#include "glyphs.h"
#include <ux_flow_engine.h>

// Enough for every optional screen + confirm/deny
#define MAX_FLOW_STEPS 15

static char g_operation_type[12];
static char g_value_str[32];
static char g_unstake_value_str[32];
static char g_fee_str[32];
static char g_core_fee_str[8];
static char g_delegator_str[41];
static char g_validator_str[41];
static char g_chain_id_str[10];
static char g_locktime_str[DATETIME_STR_LEN];

UX_STEP_NOCB(ux_tx_type_step,
             bnnn_paging,
             {.title = "Transaction type", .text = g_operation_type});

UX_STEP_NOCB(ux_tx_stake_amount_step,
             bnnn_paging,
             {.title = "Stake amount", .text = g_value_str});

UX_STEP_NOCB(ux_tx_unstake_amount_step,
             bnnn_paging,
             {.title = "Unstake amount", .text = g_unstake_value_str});

UX_STEP_NOCB(ux_tx_delegator_step,
             bnnn_paging,
             {.title = "Delegator", .text = g_delegator_str});

UX_STEP_NOCB(ux_tx_validator_step,
             bnnn_paging,
             {.title = "Validator", .text = g_validator_str});

UX_STEP_NOCB(ux_tx_network_step,
             bnnn_paging,
             {.title = "Network", .text = g_chain_id_str});

UX_STEP_NOCB(ux_tx_locktime_step,
             bnnn_paging,
             {.title = "Locktime (UTC+0)", .text = g_locktime_str});

UX_STEP_NOCB(ux_tx_core_fee_step,
             bnnn_paging,
             {.title = "Core fee", .text = g_core_fee_str});

UX_STEP_NOCB(ux_tx_fee_step,
             bnnn_paging,
             {.title = "Fee", .text = g_fee_str});

UX_STEP_CB(ux_confirm_step,
           pb,
           set_ux_flow_response(true),
           {
               &C_icon_validate_14,
               "Approve",
           });

UX_STEP_CB(ux_reject_step,
           pb,
           set_ux_flow_response(false),
           {
               &C_icon_crossmark,
               "Reject",
           });

static const ux_flow_step_t *g_flow[MAX_FLOW_STEPS];

bool display_transaction(dispatcher_context_t *dc,
                         int64_t              value_spent,
                         uint64_t             fee,
                         core_dao_tx_info_t  *info)
{
    // Format strings
    const uint64_t value_abs = (value_spent < 0) ? -value_spent : value_spent;

    format_sats_amount(COIN_COINID_SHORT, value_abs,                 g_value_str);
    format_sats_amount(COIN_COINID_SHORT, fee,                       g_fee_str);
    format_sats_amount(COIN_COINID_SHORT, info->unlock_amount,       g_unstake_value_str);
    snprintf(g_core_fee_str, sizeof(g_core_fee_str), "%u", info->fee);

    buffer_to_hex(info->delegator, 20, g_delegator_str, 41);
    buffer_to_hex(info->validator, 20, g_validator_str, 41);
    timestamp_to_string(info->locktime, g_locktime_str);

    strcpy(g_chain_id_str,
        info->chain_id == CHAID_ID_MAINNET ? "Mainnet"   :
        info->chain_id == CHAIN_ID_TESTNET ? "Testnet"   :
        info->chain_id == CHAIN_ID_TESTNET2? "Testnet2"  : "Unknown");

    strcpy(g_operation_type,
        (info->type & TYPE_TX_LOCK && info->type & TYPE_TX_UNLOCK) ? "Restake" :
        (info->type & TYPE_TX_LOCK)                                 ? "Stake"   :
                                                                      "Unstake");

    // Assemble the UX flow
    size_t i = 0;
    g_flow[i++] = &ux_tx_type_step;

    if (info->type & TYPE_TX_LOCK)
        g_flow[i++] = &ux_tx_stake_amount_step;

    if (info->type & TYPE_TX_UNLOCK)
        g_flow[i++] = &ux_tx_unstake_amount_step;

    if (info->type & TYPE_TX_LOCK) {
        g_flow[i++] = &ux_tx_delegator_step;
        g_flow[i++] = &ux_tx_validator_step;
        g_flow[i++] = &ux_tx_network_step;
        g_flow[i++] = &ux_tx_locktime_step;
        g_flow[i++] = &ux_tx_core_fee_step;
    }

    g_flow[i++] = &ux_tx_fee_step;
    g_flow[i++] = &ux_confirm_step;
    g_flow[i++] = &ux_reject_step;
    g_flow[i++] = FLOW_END_STEP;

    ux_flow_init(0, g_flow, NULL);

    // blocking call until the user approves or rejects the transaction
    bool approved = io_ui_process(dc);
    if (!approved) {
        SEND_SW(dc, SW_DENY);
        return false;
    }
    return true;
}

#endif // HAVE_BAGL
