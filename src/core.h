#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MAX_DERIVATION_PATH_DEPTH 4

#define REDEEM_SCRIPT_LEN 32
#define SCRIPT_HASH_LEN 32
#define LOCK_SCRIPT_LEN 34
#define CHAID_ID_MAINNET 1116
#define CHAIN_ID_TESTNET 1115
#define CHAIN_ID_TESTNET2 1114

#define H 0x80000000

// Useful OP_CODES
#define OP_PUSHBYTES_4 4
#define OP_PUSHBYTES_20 20
#define OP_PUSHBYTES_32 32

#define CORE_DERIVATION_PATH {84 | H, 1 | H, 0 | H, 0, 0}
#define CORE_DERIVATION_PATH_LEN 5

typedef enum {
    TYPE_TX_UNKNOWN = 0,
    TYPE_TX_LOCK = 1,
    TYPE_TX_UNLOCK = 1 << 1,
    TYPE_TX_INVALID = 1 << 2,
} tx_type_t;

typedef struct {
    // Global information
    tx_type_t type;

    // Stake information
    uint16_t chain_id;
    uint8_t delegator[20];
    uint8_t validator[20];
    uint64_t lock_amount;
    uint8_t fee;
    uint32_t locktime;

    // Unstake information
    uint64_t unlock_amount;
    uint32_t n_core_dao_inputs;
    uint8_t  core_inputs[64];
} core_dao_tx_info_t;

/***
 * Parse the staking information from an OP_RETURN output script
 * @param payload The payload of the OP_RETURN output script minus the OP_RETURN opcode and the data length
 * @param payload_len The length of the payload (should be 80 bytes)
 * @param info The parsed staking information
 * @param redeem_script The redeem script parsed from the payload

 * @return true if the parsing was successful, false otherwise
 */
bool parse_staking_information(
    uint8_t *payload,
    uint32_t payload_len,
    core_dao_tx_info_t *info,
    uint8_t redeem_script[static REDEEM_SCRIPT_LEN]
);

bool validate_redeem_script(uint8_t redeem_script[static REDEEM_SCRIPT_LEN]);

bool validate_lock_script_pubkey(
    uint8_t *lock_script_pubkey,
    size_t lock_script_pubkey_len,
    uint8_t redeem_script[static REDEEM_SCRIPT_LEN]
);

bool get_core_compressed_pubkey(uint8_t pubkey[static 33]);

bool get_core_pubkey_hash160(uint8_t hash160[static 20]);

bool get_core_redeem_script( uint32_t locktime, uint8_t redeem_script[static REDEEM_SCRIPT_LEN]);

void buffer_to_hex(const uint8_t *buffer, size_t buffer_len, char *out, size_t out_len);
