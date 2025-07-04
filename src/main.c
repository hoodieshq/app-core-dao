#include <stdbool.h>

#include "../bitcoin_app_base/src/boilerplate/dispatcher.h"
#include "../bitcoin_app_base/src/common/bitvector.h"
#include "../bitcoin_app_base/src/common/psbt.h"
#include "../bitcoin_app_base/src/handler/lib/get_merkleized_map.h"
#include "../bitcoin_app_base/src/handler/lib/get_merkleized_map_value.h"
#include "../bitcoin_app_base/src/handler/sign_psbt.h"
#include "../bitcoin_app_base/src/handler/sign_psbt/txhashes.h"
#include "../bitcoin_app_base/src/crypto.h"
#include "../bitcoin_app_base/src/handler/sign_psbt/extract_bip32_derivation.h"

#include "display.h"
#include "debug.h"
#include "core.h"

#define SCRIPT_PUBKEY_BUFFER_LEN 83  // Max length for OP_RETURN scriptPubKey
#define P2TR_SCRIPTPUBKEY_LEN    34

static core_dao_tx_info_t core_tx_info;

// No custom APDU
bool custom_apdu_handler(dispatcher_context_t *dc, const command_t *cmd) {
    UNUSED(dc), UNUSED(cmd);
    return false;
}

static bool get_output_amount(dispatcher_context_t *dc,
                              merkleized_map_commitment_t *map,
                              uint64_t *amount) {
    uint8_t raw_amount[8];
    if (8 != call_get_merkleized_map_value(dc,
                                           map,
                                           (uint8_t[]){PSBT_OUT_AMOUNT},
                                           sizeof((uint8_t[]){PSBT_OUT_AMOUNT}),
                                           raw_amount,
                                           sizeof(raw_amount))) {
        SEND_SW(dc, SW_INCORRECT_DATA);
        return false;
    }
    *amount = read_u64_le(raw_amount, 0);
    return true;
}

static uint64_t read_amount(const uint8_t *buffer) {
    uint64_t amount = 0;
    amount |= (uint64_t) buffer[0] << 0;
    amount |= (uint64_t) buffer[1] << 8;
    amount |= (uint64_t) buffer[2] << 16;
    amount |= (uint64_t) buffer[3] << 24;
    amount |= (uint64_t) buffer[4] << 32;
    amount |= (uint64_t) buffer[5] << 40;
    amount |= (uint64_t) buffer[6] << 48;
    amount |= (uint64_t) buffer[7] << 56;
    return amount;
}

static bool get_utxo_witness(dispatcher_context_t *dc,
                             merkleized_map_commitment_t *map,
                             uint64_t *amount,
                             uint8_t script_pubkey[static SCRIPT_HASH_LEN]) {
    uint8_t utxo[43];  // 8 bytes amount; 1 byte length; 34 bytes P2TR Script
    if (sizeof(utxo) != call_get_merkleized_map_value(dc,
                                                      map,
                                                      (uint8_t[]){PSBT_IN_WITNESS_UTXO},
                                                      sizeof((uint8_t[]){PSBT_IN_WITNESS_UTXO}),
                                                      utxo,
                                                      sizeof(utxo))) {
        PRINT("Unable to get witness UTXO or invalid witness UTXO\n");
        return false;
    }
    *amount = read_amount(utxo);
    if (utxo[8 + 0] != 34 || utxo[8 + 1] != OP_0 || utxo[8 + 2] != OP_PUSHBYTES_32) {
        PRINT("Unexpected scriptPubKey length in witness UTXO: %d\n", utxo[0]);
        return false;
    }
    memcpy(script_pubkey, utxo + 8 + 3, SCRIPT_HASH_LEN);
    return true;
}

static bool get_script_pubkey(dispatcher_context_t *dc,
                              merkleized_map_commitment_t *map,
                              uint8_t *script_pubkey,
                              size_t script_pubkey_len,
                              int *result_len) {
    *result_len = call_get_merkleized_map_value(dc,
                                                map,
                                                (uint8_t[]){PSBT_OUT_SCRIPT},
                                                1,
                                                script_pubkey,
                                                script_pubkey_len);
    if (*result_len < 0) {
        SEND_SW(dc, SW_INCORRECT_DATA);
        return false;
    }
    return true;
}

static bool get_input_redeem_script(dispatcher_context_t *dc,
                                    merkleized_map_commitment_t *map,
                                    uint8_t *redeem_script,
                                    size_t redeem_script_len) {
    if (REDEEM_SCRIPT_LEN !=
        call_get_merkleized_map_value(dc,
                                      map,
                                      (uint8_t[]){PSBT_IN_WITNESS_SCRIPT},
                                      sizeof((uint8_t[]){PSBT_IN_WITNESS_SCRIPT}),
                                      redeem_script,
                                      redeem_script_len)) {
        SEND_SW(dc, SW_INCORRECT_DATA);
        return false;
    }
    return true;
}

typedef struct {
    bool found;
    size_t bip32_der_len;
    uint32_t bip32_der[MAX_BIP32_PATH_STEPS];
} input_bip32_path_t;

static void input_keys_callback(dispatcher_context_t *dc,
                                input_bip32_path_t *callback_data,
                                const merkleized_map_commitment_t *map_commitment,
                                int index,
                                buffer_t *data) {
    size_t data_len = data->size - data->offset;
    uint32_t fpt_der[1 + MAX_BIP32_PATH_STEPS];

    if (data_len >= 1) {
        uint8_t key_type;
        buffer_read_u8(data, &key_type);
        if (!callback_data->found && key_type == PSBT_IN_BIP32_DERIVATION) {
            int der_len = extract_bip32_derivation(dc,
                                                   key_type,
                                                   map_commitment->values_root,
                                                   map_commitment->size,
                                                   index,
                                                   fpt_der);

            if (der_len < 0) {
                PRINT("Failed to read BIP32_DERIVATION\n");
                return;
            }

            if (der_len < 2 || der_len > MAX_BIP32_PATH_STEPS) {
                PRINT("BIP32_DERIVATION path too long\n");
                return;
            }

            callback_data->found = true;
            callback_data->bip32_der_len = der_len;
            for (int i = 0; i < der_len; i++) {
                // Skip the fingerprint
                callback_data->bip32_der[i] = fpt_der[i + 1];
            }
        }
    }
}

static tx_type_t validate_lock_transaction(dispatcher_context_t *dc,
                                           sign_psbt_state_t *st,
                                           const uint8_t internal_outputs[64],
                                           core_dao_tx_info_t *info) {
    merkleized_map_commitment_t external_output_map;
    uint8_t script_pubkey[SCRIPT_PUBKEY_BUFFER_LEN];
    int script_pubkey_len;
    uint8_t redeem_script[REDEEM_SCRIPT_LEN];
    uint8_t lock_script_pubkey[LOCK_SCRIPT_LEN];
    input_bip32_path_t input_bip32_path = {0};

    // Iterate through all inputs
    for (unsigned int cur_input_index = 0; cur_input_index < st->n_inputs; cur_input_index++) {
        input_info_t input = {0};

        int res = call_get_merkleized_map_with_callback(
            dc,
            (void *) &input_bip32_path,
            st->inputs_root,
            st->n_inputs,
            cur_input_index,
            (merkle_tree_elements_callback_t) input_keys_callback,
            &input.in_out.map);

        if (res < 0) {
            PRINT("Failed to process input map\n");
            SEND_SW(dc, SW_INCORRECT_DATA);
            return TYPE_TX_INVALID;
        }
    }

    // Iterate through all outputs
    for (unsigned int i = 0; i < st->n_outputs; i++) {
        PRINT("Checking output %d\n", i);
        if (call_get_merkleized_map(dc, st->outputs_root, st->n_outputs, i, &external_output_map) <
            0) {
            PRINT("Failed to get input\n");
            return TYPE_TX_INVALID;
        }
        // Get output amount
        uint64_t amount;
        if (!get_output_amount(dc, &external_output_map, &amount)) {
            return TYPE_TX_INVALID;
        }
        // Get output scriptPubKey
        if (!get_script_pubkey(dc,
                               &external_output_map,
                               script_pubkey,
                               sizeof(script_pubkey),
                               &script_pubkey_len)) {
            PRINT("Failed to get scriptPubKey for output %d\n", i);
            return TYPE_TX_INVALID;
        }
        if (script_pubkey[0] == OP_RETURN) {
            if (!parse_staking_information(script_pubkey + 3,
                                           script_pubkey_len - 3,
                                           info,
                                           redeem_script) ||
                amount != 0) {
                PRINT_HEX(script_pubkey, script_pubkey_len, "OP_RETURN scriptPubKey: ");
                PRINT("Invalid OP_RETURN output or amount is not at zero\n");
                SEND_SW(dc, SW_INCORRECT_DATA);
                return TYPE_TX_INVALID;
            }

            if (info->found_outputs.is_stacking_info_output_found) {
                PRINT("Found duplicated OP_RETURN output\n");
                SEND_SW(dc, SW_INCORRECT_DATA);
                return TYPE_TX_INVALID;
            }

            info->found_outputs.is_stacking_info_output_found = true;
        } else if (input_bip32_path.found && check_if_change_output(input_bip32_path.bip32_der,
                                                                    input_bip32_path.bip32_der_len,
                                                                    script_pubkey,
                                                                    script_pubkey_len)) {
            if (info->found_outputs.is_unlock_or_change_output_found) {
                SEND_SW(dc, SW_INCORRECT_DATA);
                return TYPE_TX_INVALID;
            }

            PRINT("Found change output %d\n", i);
            info->found_outputs.is_unlock_or_change_output_found = true;
            st->outputs.change_total_amount += amount;
        } else if (bitvector_get(internal_outputs, i) == 1) {
            // Error only in case duplicate change output is found,
            // if we already found it during valaidation of unlock TX we should continue
            if (info->found_outputs.is_unlock_or_change_output_found &&
                info->found_outputs.unlock_or_change_output_num != i) {
                PRINT("Found duplicated change output (internal)\n");
                SEND_SW(dc, SW_INCORRECT_DATA);
                return TYPE_TX_INVALID;
            }

            // If the output is internal, consider it to be the change
            PRINT("Found change output %d (internal)\n", i);
            info->found_outputs.is_unlock_or_change_output_found = true;
        } else {
            if (script_pubkey_len != LOCK_SCRIPT_LEN) {
                PRINT("Invalid scriptPubKey length for locking output (%d)\n", i);
                SEND_SW(dc, SW_INCORRECT_DATA);
                return TYPE_TX_INVALID;
            }

            if (info->found_outputs.is_lock_output_found) {
                PRINT("Found duplicated lock output\n");
                SEND_SW(dc, SW_INCORRECT_DATA);
                return TYPE_TX_INVALID;
            }

            memcpy(lock_script_pubkey, script_pubkey, LOCK_SCRIPT_LEN);
            info->found_outputs.is_lock_output_found = true;
        }
    }

    // If a lock output and a valid op return was found the tx is a staking tx
    if (info->found_outputs.is_stacking_info_output_found &&
        info->found_outputs.is_lock_output_found) {
        PRINT("Staking transaction\n");
        info->type |= TYPE_TX_LOCK;
    } else {
        PRINT("NOT A STAKING TX\n");
        return info->type;
    }

    if (info->type & TYPE_TX_LOCK && (st->n_outputs < 2 || st->n_outputs > 3)) {
        PRINT("Invalid number of outputs\n");
        SEND_SW(dc, SW_INCORRECT_DATA);
        return TYPE_TX_INVALID;
    }

    info->lock_amount = st->internal_inputs_total_amount - st->outputs.change_total_amount;

    PRINT_HEX(info->delegator, 20, "Delegator: ");
    PRINT_HEX(info->validator, 20, "Validator: ");
    PRINT("Amount: %llu\n", info->lock_amount);
    PRINT("Total Amount: %llu\n", st->internal_inputs_total_amount);
    PRINT("Change: %llu\n", st->outputs.change_total_amount);
    PRINT("Fee: %d\n", info->fee);
    PRINT_HEX(redeem_script, REDEEM_SCRIPT_LEN, "Redeem script: ");
    PRINT_HEX(lock_script_pubkey, LOCK_SCRIPT_LEN, "Lock scriptPubKey: ");

    // Verify the redeem script contains the expected public key
    if (!validate_redeem_script(redeem_script)) {
        PRINT("Invalid redeem script in OP_RETURN output\n");
        SEND_SW(dc, SW_INCORRECT_DATA);
        return TYPE_TX_INVALID;
    }

    // Verify the lock output uses the right redeem script
    if (!validate_lock_script_pubkey(lock_script_pubkey, LOCK_SCRIPT_LEN, redeem_script)) {
        PRINT("Invalid scriptPubKey for the lock output\n");
        SEND_SW(dc, SW_INCORRECT_DATA);
        return TYPE_TX_INVALID;
    }

    return info->type;
}

static tx_type_t validate_unlock_transaction(dispatcher_context_t *dc,
                                             sign_psbt_state_t *st,
                                             const uint8_t internal_inputs[64],
                                             const uint8_t internal_outputs[64],
                                             core_dao_tx_info_t *info) {
    merkleized_map_commitment_t external_input_map;
    // Count the number of CoreDAO inputs
    info->n_core_dao_inputs = 0;
    info->unlock_amount = 0;
    for (unsigned int i = 0; i < st->n_inputs; i++) {
        PRINT("Checking input %d\n", i);
        if (bitvector_get(internal_inputs, i) == 0) {
            // Verify if the input is a CoreDAO input (fail otherwise)
            // Get commitment to the i-th input's map
            PRINT("Getting input %d\n", i);
            if (call_get_merkleized_map(dc, st->inputs_root, st->n_inputs, i, &external_input_map) <
                0) {
                PRINT("Failed to get input &d\n", i);
                return TYPE_TX_INVALID;
            }
            // Get input amount and redeem script
            uint64_t amount;
            uint8_t redeem_script_hash[SCRIPT_HASH_LEN];
            uint8_t redeem_script[REDEEM_SCRIPT_LEN];

            if (!get_utxo_witness(dc, &external_input_map, &amount, redeem_script_hash)) {
                return TYPE_TX_INVALID;
            }

            if (!get_input_redeem_script(dc,
                                         &external_input_map,
                                         redeem_script,
                                         REDEEM_SCRIPT_LEN)) {
                return TYPE_TX_INVALID;
            }

            // Check if the redeem script is a valid CoreDAO redeem script
            if (!validate_redeem_script(redeem_script)) {
                PRINT("Invalid redeem script in input %d\n", i);
                return TYPE_TX_INVALID;
            }

            info->type |= TYPE_TX_UNLOCK;
            info->n_core_dao_inputs += 1;
            info->unlock_amount += amount;
            bitvector_set(info->core_inputs, i, 1);  // Mark the input as a CoreDAO input
        } else {
            PRINT("Internal input %d\n", i);
        }
    }

    // Find unlock output
    for (unsigned int i = 0; i < st->n_outputs; i++) {
        if (bitvector_get(internal_outputs, i) == 1) {
            if (info->found_outputs.is_unlock_or_change_output_found) {
                PRINT("Found duplicated unlock output\n");
                SEND_SW(dc, SW_INCORRECT_DATA);
                return TYPE_TX_INVALID;
            }

            PRINT("Found unlock output %d\n", i);
            info->found_outputs.is_unlock_or_change_output_found = true;
            info->found_outputs.unlock_or_change_output_num = i;
        }
    }

    PRINT("Unlock amount: %llu\n", info->unlock_amount);

    return info->type;
}

static tx_type_t validate_found_outputs_for_tx_type(core_dao_tx_info_t *info) {
    if (info->type & TYPE_TX_INVALID) {
        return info->type;
    }

    if (info->type & TYPE_TX_UNLOCK && info->type & TYPE_TX_LOCK) {
        if (info->found_outputs.is_unlock_or_change_output_found) {
            PRINT("Forbidden unlock or change output in lock/unlock TX\n");
            info->type |= TYPE_TX_INVALID;
        }

        return info->type;

    } else if (info->type & TYPE_TX_UNLOCK) {
        if (!info->found_outputs.is_unlock_or_change_output_found) {
            PRINT("Missing unlock output for unlock TX\n");
            info->type |= TYPE_TX_INVALID;
        }
    }

    return info->type;
}

static tx_type_t validate_transaction(dispatcher_context_t *dc,
                                      sign_psbt_state_t *st,
                                      const uint8_t internal_inputs[64],
                                      const uint8_t internal_outputs[64],
                                      core_dao_tx_info_t *info) {
    // This application implements the following rules:
    // For lock TX:
    // - If a transaction contains a OP_RETURN output, It must be a valid CoreDAO output
    // - If a transaction contains a OP_RETURN output, It must have a locking output
    // - The PSBT can have at most 1 change output
    // - The PSBT can have any number of CoreDAO inputs
    // - The PSBT can have any number of internal inputs
    // - If at least one input is a CoreDAO input, outputs can only be change or lock output
    // For unlock TX:
    // - If a transaction contains a spending CLTV UTXO input, it must be a valid CoreDao unlock TX
    // - If a transaction contains a spending CLTV UTXO input, it must have 1 unlocking output
    // For combined unlock/lock (restake) TX:
    // - All rules for both lock and unlock transactions apply, except that such a
    //   transaction should not have a change or unlocking output.

    tx_type_t tx_type = TYPE_TX_UNKNOWN;
    PRINT("Validating transaction\n");
    tx_type |= validate_unlock_transaction(dc, st, internal_inputs, internal_outputs, info);
    if (!(tx_type & TYPE_TX_INVALID)) {
        tx_type |= validate_lock_transaction(dc, st, internal_outputs, info);
    }

    tx_type |= validate_found_outputs_for_tx_type(info);

    return tx_type;
}

// hooking into a weak function
bool validate_and_display_transaction(dispatcher_context_t *dc,
                                      sign_psbt_state_t *st,
                                      const uint8_t internal_inputs[64],
                                      const uint8_t internal_outputs[64]) {
    PRINT("Validating and displaying transaction\n");

    explicit_bzero(&core_tx_info, sizeof(core_tx_info));

    tx_type_t tx_type =
        validate_transaction(dc, st, internal_inputs, internal_outputs, &core_tx_info);

    if ((tx_type & TYPE_TX_INVALID) == TYPE_TX_INVALID) {
        PRINT("Send invalid status\n");
        SEND_SW(dc, SW_INCORRECT_DATA);
        return false;
    }

    uint64_t fee = st->inputs_total_amount - st->outputs.total_amount;

    // The amount spent from the wallet policy (or negative if the it received more funds than it
    // spent)
    int64_t internal_value = st->internal_inputs_total_amount + core_tx_info.unlock_amount -
                             st->outputs.change_total_amount - fee;

    if (!display_transaction(dc, internal_value, fee, &core_tx_info)) {
        return false;
    }

    return true;
}

/**
 * @brief Signs the custom (special) input.
 *
 * This function must be implemented in order to sign for all the inputs that are not internal.
 * If not implemented, only the internal inputs are signed (handled by the base app).
 *
 * This function must return false in case of any error. In that case, an error status word should
 * be sent. If the function returns true, no status word should be sent.
 *
 * @param dc Dispatcher context.
 * @param st PSBT signing state.
 * @param tx_hashes Transaction hashes.
 * @param internal_inputs Bitvector representing internal inputs.
 * @return true if signing was successful, false otherwise.
 */
bool sign_custom_inputs(
    dispatcher_context_t *dc,
    sign_psbt_state_t *st,
    tx_hashes_t *tx_hashes,
    const uint8_t internal_inputs[static BITVECTOR_REAL_SIZE(MAX_N_INPUTS_CAN_SIGN)]) {
    UNUSED(dc), UNUSED(st), UNUSED(tx_hashes), UNUSED(internal_inputs);
    merkleized_map_commitment_t input_map;
    uint8_t sighash[32];
    uint32_t path[] = CORE_DERIVATION_PATH;
    uint64_t amount;
    uint8_t script_pubkey[2 + SCRIPT_HASH_LEN];

    for (size_t i = 0; i < st->n_inputs; i++) {
        if (bitvector_get(core_tx_info.core_inputs, i) == 1) {
            PRINT("Signing input %d\n", i);
            // Get the commitment to the i-th input's map
            if (call_get_merkleized_map(dc, st->inputs_root, st->n_inputs, i, &input_map) < 0) {
                PRINT("Failed to get input &d\n", i);
                SEND_SW(dc, SW_INCORRECT_DATA);
                return false;
            }
            // Get the redeem script
            if (!get_utxo_witness(dc, &input_map, &amount, script_pubkey + 2)) {
                SEND_SW(dc, SW_INCORRECT_DATA);
                return false;
            }
            script_pubkey[0] = 0x00;
            script_pubkey[1] = 0x20;
            PRINT_HEX(script_pubkey, SCRIPT_HASH_LEN + 2, "Redeem script: ");
            if (!compute_sighash_segwitv0(dc,
                                          st,
                                          tx_hashes,
                                          &input_map,
                                          i,
                                          script_pubkey,
                                          SCRIPT_HASH_LEN + 2,
                                          SIGHASH_ALL,
                                          sighash)) {
                PRINT("Failed to compute the sighash\n");
                return false;
            }
            if (!sign_sighash_ecdsa_and_yield(dc,
                                              st,
                                              i,
                                              path,
                                              CORE_DERIVATION_PATH_LEN,
                                              SIGHASH_ALL,
                                              sighash)) {
                PRINT("Signing failed\n");
                return false;
            }
        }
    }
    return true;
}