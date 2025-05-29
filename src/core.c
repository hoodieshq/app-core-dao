#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "core.h"
#include "debug.h"

#include "../bitcoin_app_base/src/crypto.h"
#include "../bitcoin_app_base/src/common/script.h"
#include "../bitcoin_app_base/src/common/read.h"
#include "../bitcoin_app_base/src/common/write.h"

#include "cx.h"
#include "ledger_assert.h"

#define EXPECTED_PAYLOAD_LEN 80 // SAT+(4)+ VERSION(1) + CHAIN_ID(2) + DELEGATOR(20) + VALIDATOR(20) + FEE(1) + REDEEM(32)

#define SUPPORTED_VERSION 1



static const char *SAT_PLUS = "SAT+";

bool parse_staking_information(
    uint8_t *payload,
    uint32_t payload_len,
    core_dao_tx_info_t *info,
    uint8_t redeem_script[static REDEEM_SCRIPT_LEN]
) {
    if (payload_len != EXPECTED_PAYLOAD_LEN) {
        PRINT("Expected payload length %d, got %d\n", EXPECTED_PAYLOAD_LEN, payload_len);
        return false;
    }

    // Read SAT+
    if (memcmp(payload, SAT_PLUS, 4) != 0) {
        PRINT("Invalid SAT+ prefix\n");
        return false;
    }
    payload += 4;

    // Read version
    if (*payload != SUPPORTED_VERSION) {
        PRINT("Unsupported version %d\n", *payload);
        return false;
    }
    payload++;

    // Read chain id
    info->chain_id = (payload[0] << 8) | payload[1];
    if (info->chain_id != CHAID_ID_MAINNET &&
        info->chain_id != CHAIN_ID_TESTNET &&
        info->chain_id != CHAIN_ID_TESTNET2) {
        PRINT("Unsupported chain id %d\n", info->chain_id);
        return false;
    }
    payload += 2;

    // Read delegator
    memcpy(info->delegator, payload, 20);
    payload += 20;

    // Read validator
    memcpy(info->validator, payload, 20);
    payload += 20;

    // Read fee
    info->fee = *payload;
    payload++;

    // Read redeem script
    memcpy(redeem_script, payload, REDEEM_SCRIPT_LEN);

    // Read locktime
    info->locktime = read_u32_le(redeem_script, 1);
    return true;
}

bool get_core_redeem_script(uint32_t locktime, uint8_t redeem_script[static REDEEM_SCRIPT_LEN]) {
    int offset = 0;

    redeem_script[offset++] = OP_PUSHBYTES_4;
    write_u32_le(redeem_script, offset, locktime);
    offset += 4;
    redeem_script[offset++] = OP_CHECKLOCKTIMEVERIFY;
    redeem_script[offset++] = OP_DROP;
    redeem_script[offset++] = OP_DUP;
    redeem_script[offset++] = OP_HASH160;
    redeem_script[offset++] = OP_PUSHBYTES_20; // Push 20 bytes
    if (!get_core_pubkey_hash160(redeem_script + offset)) {
        return false;
    }
    offset += 20;
    redeem_script[offset++] = OP_EQUALVERIFY;
    redeem_script[offset++] = OP_CHECKSIG;
    return true;
}

bool validate_redeem_script(uint8_t redeem_script[static REDEEM_SCRIPT_LEN]) {
    uint8_t expected_redeem_script[REDEEM_SCRIPT_LEN];
    uint32_t locktime;

    // Read locktime from redeem script
    locktime = read_u32_le(redeem_script, 1);

    if (!get_core_redeem_script(locktime, expected_redeem_script)) {
        return false;
    }
    return memcmp(redeem_script, expected_redeem_script, REDEEM_SCRIPT_LEN) == 0;
}

bool validate_lock_script_pubkey(
    uint8_t *lock_script_pubkey,
    size_t lock_script_pubkey_len,
    uint8_t *redeem_script
) {
    uint8_t expected_redeem_script[REDEEM_SCRIPT_LEN];
    uint8_t script_hash[32];
    uint32_t locktime; 

    // Read locktime from redeem script
    locktime = read_u32_le(redeem_script, 1);

    if (!get_core_redeem_script(locktime, expected_redeem_script)) {
        return false;
    }
    
    if (cx_hash_sha256(expected_redeem_script, REDEEM_SCRIPT_LEN, script_hash, 32) != 32) {
        return false;
    }

    if (lock_script_pubkey_len != LOCK_SCRIPT_LEN) {
        return false;
    }
    if (lock_script_pubkey[0] != OP_0 || lock_script_pubkey[1] != OP_PUSHBYTES_32) {
        return false;
    }

    return memcmp(lock_script_pubkey + 2, script_hash, 32) == 0;
}

bool get_core_compressed_pubkey(uint8_t pubkey[static 33]) {
    uint32_t path[] = CORE_DERIVATION_PATH;
    uint8_t chaincode[32];
    
    return crypto_get_compressed_pubkey_at_path(path, CORE_DERIVATION_PATH_LEN, pubkey, chaincode);
}

bool get_core_pubkey_hash160(uint8_t hash160[static 20]) {
    uint8_t pubkey[33];
    if (!get_core_compressed_pubkey(pubkey)) {
        return false;
    }
    crypto_hash160(pubkey, 33, hash160);
    return true;
}

void buffer_to_hex(const uint8_t *buffer,
                   size_t          buffer_len,
                   char           *out,
                   size_t          out_len)
{
    static const char lut[] = "0123456789abcdef";

    // Each byte needs two chars. Reserve one byte for '\0'
    size_t max_bytes = (out_len - 1) / 2;

    size_t i;
    for (i = 0; i < buffer_len && i < max_bytes; ++i) {
        uint8_t byte = buffer[i];
        out[i * 2]     = lut[byte >> 4];   /* high nibble */
        out[i * 2 + 1] = lut[byte & 0x0F]; /* low  nibble */
    }

    // Always terminate the string
    out[i * 2] = '\0';
}