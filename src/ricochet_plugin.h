#pragma once

#include "eth_internals.h"
#include "eth_plugin_interface.h"
#include <string.h>

#define NUM_SELECTORS        6
#define PLUGIN_NAME          "Ricochet"
#define SELECTOR_SIZE        4
#define TOKEN_FOUND          1 << 1
#define PARAMETER_LENGTH     32
#define RUN_APPLICATION      1
#define TOKEN_SENT_FOUND     1
#define TOKEN_RECEIVED_FOUND 1 << 1
#define DEFAULT_TICKER       "MATIC"
#define METHOD_NAME_LENGTH   20

#define NUM_SUPER_TOKEN_COLLECTION      9
#define NUM_CONTRACT_ADDRESS_COLLECTION 15
#define NUM_CFA_METHOD_COLLECTION       3

typedef enum {
    DOWNGRADE,
    DOWNGRADE_TO_ETH,
    CALL_AGREEMENT,
    UPGRADE,
    UPGRADE_TO_ETH,
    BATCH_CALL
} selector_t;

// Enumeration used to parse the smart contract data.
#define AMOUNT               0
#define NONE                 1
#define AGREEMENT_CLASS      2
#define PATH_OFFSET          3
#define USER_DATA            4
#define PATH_LENGTH          5
#define CALL_DATA            6
#define CONTRACT_PATH_OFFSET 7
#define OPERATION_TYPE       8
#define TARGET               9
#define BYTES_ARRAY_LEN      10
#define INPUT_DATA           11

#define START_STREAM  0
#define UPDATE_STREAM 1
#define STOP_STREAM   2

typedef enum { SEND_SCREEN, RECEIVE_SCREEN, ERROR } screens_t;

extern const uint8_t *const RICOCHET_SELECTORS[NUM_SELECTORS];

// Number of decimals used when the token wasn't found in the CAL.
#define DEFAULT_DECIMAL WEI_TO_ETHER

// uses `0xeeeee` as a dummy address to represent ETH.
extern const uint8_t RICOCHET_ETH_ADDRESS[ADDRESS_LENGTH];

// Adress 0x00000... used to indicate that the beneficiary is the sender.
extern const uint8_t NULL_ETH_ADDRESS[ADDRESS_LENGTH];

// Returns 1 if corresponding address is the Ricochet address for the chain token (ETH, BNB, MATIC,
// etc.. are 0xeeeee...).
#define ADDRESS_IS_NETWORK_TOKEN(_addr) (!memcmp(_addr, RICOCHET_ETH_ADDRESS, ADDRESS_LENGTH))

typedef struct super_token_ticker {
    uint8_t super_token_address[ADDRESS_LENGTH];
    char ticker_token[MAX_TICKER_LEN];
    char ticker_super_token[MAX_TICKER_LEN];

} super_token_ticker_t;

extern const super_token_ticker_t SUPER_TOKEN_COLLECTION[NUM_SUPER_TOKEN_COLLECTION];

typedef struct contract_address_ticker {
    uint8_t contract_address[ADDRESS_LENGTH];
    char ticker_sent[MAX_TICKER_LEN];
    char ticker_received[MAX_TICKER_LEN];

} contract_address_ticker_t;
extern const contract_address_ticker_t CONTRACT_ADDRESS_COLLECTION[NUM_CONTRACT_ADDRESS_COLLECTION];

typedef struct cfa_method {
    uint8_t method[SELECTOR_SIZE];
    char method_name[METHOD_NAME_LENGTH];
    uint8_t method_id;
} cfa_method_t;
extern const cfa_method_t CFA_METHOD_COLLECTION[NUM_CFA_METHOD_COLLECTION];

typedef struct context_t {
    // For display.
    uint8_t amount[INT256_LENGTH];
    uint8_t contract_address_sent[ADDRESS_LENGTH];
    uint8_t contract_address_received[ADDRESS_LENGTH];
    uint8_t token_address[ADDRESS_LENGTH];
    char ticker_sent[MAX_TICKER_LEN];
    char ticker_received[MAX_TICKER_LEN];
    uint8_t method_cfa[SELECTOR_SIZE];
    uint8_t method_id;

    uint32_t offset;
    uint16_t go_to_offset;
    uint16_t checkpoint;
    uint8_t next_param;
    uint8_t tokens_found;
    uint8_t valid;
    uint8_t decimals;
    uint8_t decimals_received;
    uint8_t selectorIndex;
    uint16_t array_len;
    uint8_t skip;
} context_t;

// Piece of code that will check that the above structure is not bigger than 5 * 32. Do not remove
// this check.
_Static_assert(sizeof(context_t) <= 5 * 32, "Structure of parameters too big.");

char compare_array(const uint8_t a[], const uint8_t b[], size_t size);
