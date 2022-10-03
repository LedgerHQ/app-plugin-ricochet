#include "ricochet_plugin.h"
#include <limits.h>

#ifdef TARGET_TESTING
#include <bsd/string.h>
#endif

// Set UI for the "Send" screen.

// function to compare array elements
char compare_array(const uint8_t a[], const uint8_t b[], size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (a[i] != b[i]) return 1;
    }
    return 0;
}

static int amountToDecimal(const context_t *context, size_t size, unsigned long long *out) {
    unsigned long long value = 0;
    for (size_t i = 0; i < size; i++) {
        if (value > ULLONG_MAX / 256) {
            return -1;
        }
        value = value * 256 + context->amount[i];
    }
    *out = value;
    return 0;
}

static void decimalToAmount(unsigned long long value, context_t *context) {
    int i = 0;
    uint8_t rem;
    memset(context->amount, 0, sizeof(context->amount));
    do {
        rem = (uint8_t) (value % 256);
        value /= 256;
        context->amount[sizeof(context->amount) - i - 1] = rem;
        i++;
    } while (value != 0);
}

static void set_amount_ui(ethQueryContractUI_t *msg, const context_t *context) {
    strlcpy(msg->title, "Send", msg->titleLength);

    amountToString(context->amount,
                   sizeof(context->amount),
                   DEFAULT_DECIMAL,
                   context->ticker_sent,
                   msg->msg,
                   msg->msgLength);
}

static int set_cfa_from_ui(ethQueryContractUI_t *msg, context_t *context) {
    strlcpy(msg->title, "From", msg->titleLength);

    contract_address_ticker_t *currentTicker = NULL;

    for (int i = 0; i < NUM_CONTRACT_ADDRESS_COLLECTION; i++) {
        currentTicker = (contract_address_ticker_t *) PIC(&CONTRACT_ADDRESS_COLLECTION[i]);
        if (compare_array(currentTicker->contract_address,
                          context->contract_address_received,
                          ADDRESS_LENGTH) == 0) {
            strlcpy(context->ticker_sent,
                    (char *) currentTicker->ticker_sent,
                    sizeof(context->ticker_sent));
            break;
        }
    }

    if (context->method_id != STOP_STREAM) {
        unsigned long long value;
        if (amountToDecimal(context, sizeof(context->amount), &value)) {
            return -1;
        }
        // switch from token per sec to token per month for UX only.
        if (__builtin_umulll_overflow(value, 2592000, &value)) {
            return -1;
        }
        decimalToAmount(value, context);

        amountToString(context->amount,
                       sizeof(context->amount),
                       DEFAULT_DECIMAL,
                       context->ticker_sent,
                       msg->msg,
                       msg->msgLength);

        strlcat(msg->msg, " per month", msg->msgLength);
    } else {
        strlcpy(msg->msg, context->ticker_sent, msg->msgLength);
    }
    return 0;
}

static void set_cfa_to_ui(ethQueryContractUI_t *msg, context_t *context) {
    strlcpy(msg->title, "To", msg->titleLength);

    contract_address_ticker_t *currentTicker = NULL;

    for (int i = 0; i < NUM_CONTRACT_ADDRESS_COLLECTION; i++) {
        currentTicker = (contract_address_ticker_t *) PIC(&CONTRACT_ADDRESS_COLLECTION[i]);
        if (compare_array(currentTicker->contract_address,
                          context->contract_address_received,
                          ADDRESS_LENGTH) == 0) {
            strlcpy(context->ticker_received,
                    (char *) currentTicker->ticker_received,
                    sizeof(context->ticker_received));
            break;
        }
    }
    strlcpy(msg->msg, context->ticker_received, msg->msgLength);
}

static int set_batch_call_from_ui(ethQueryContractUI_t *msg, context_t *context) {
    strlcpy(msg->title, "From", msg->titleLength);

    contract_address_ticker_t *currentTicker = NULL;

    for (uint8_t i = 0; i < NUM_CONTRACT_ADDRESS_COLLECTION; i++) {
        currentTicker = (contract_address_ticker_t *) PIC(&CONTRACT_ADDRESS_COLLECTION[i]);
        if (compare_array(currentTicker->contract_address,
                          context->contract_address_received,
                          ADDRESS_LENGTH) == 0) {
            strlcpy(context->ticker_sent,
                    (char *) currentTicker->ticker_sent,
                    sizeof(context->ticker_sent));
            break;
        }
    }

    unsigned long long value;
    if (amountToDecimal(context, sizeof(context->amount), &value)) {
        return -1;
    }
    // switch from token per sec to token per month for UX only.
    if (__builtin_umulll_overflow(value, 2592000, &value)) {
        return -1;
    }
    decimalToAmount(value, context);

    amountToString(context->amount,
                   sizeof(context->amount),
                   DEFAULT_DECIMAL,
                   context->ticker_sent,
                   msg->msg,
                   msg->msgLength);

    strlcat(msg->msg, " per month", msg->msgLength);
    return 0;
}

static void set_batch_call_to_ui(ethQueryContractUI_t *msg, context_t *context) {
    strlcpy(msg->title, "To", msg->titleLength);

    contract_address_ticker_t *currentTicker = NULL;

    for (uint8_t i = 0; i < NUM_CONTRACT_ADDRESS_COLLECTION; i++) {
        currentTicker = (contract_address_ticker_t *) PIC(&CONTRACT_ADDRESS_COLLECTION[i]);
        if (compare_array(currentTicker->contract_address,
                          context->contract_address_received,
                          ADDRESS_LENGTH) == 0) {
            strlcpy(context->ticker_received,
                    (char *) currentTicker->ticker_received,
                    sizeof(context->ticker_received));
            break;
        }
    }
    strlcpy(msg->msg, context->ticker_received, msg->msgLength);
}

static void set_upgrade_to_eth_send_ui(ethQueryContractUI_t *msg, const context_t *context) {
    strlcpy(msg->title, "Send", msg->titleLength);

    amountToString(msg->pluginSharedRO->txContent->value.value,
                   msg->pluginSharedRO->txContent->value.length,
                   DEFAULT_DECIMAL,
                   context->ticker_sent,
                   msg->msg,
                   msg->msgLength);
}

static void set_upgrade_to_eth_received_ui(ethQueryContractUI_t *msg, const context_t *context) {
    strlcpy(msg->title, "Receive", msg->titleLength);

    amountToString(msg->pluginSharedRO->txContent->value.value,
                   msg->pluginSharedRO->txContent->value.length,
                   DEFAULT_DECIMAL,
                   context->ticker_received,
                   msg->msg,
                   msg->msgLength);
}

static void set_receive_ui(ethQueryContractUI_t *msg, const context_t *context) {
    strlcpy(msg->title, "Receive", msg->titleLength);
    amountToString(context->amount,
                   sizeof(context->amount),
                   DEFAULT_DECIMAL,
                   context->ticker_received,
                   msg->msg,
                   msg->msgLength);
}

// Helper function that returns the enum corresponding to the screen that should be displayed.
static screens_t get_screen(const ethQueryContractUI_t *msg) {
    uint8_t index = msg->screenIndex;

    switch (index) {
        case 0:
            return SEND_SCREEN;
        case 1:
            return RECEIVE_SCREEN;
        default:
            return ERROR;
    }
}

void handle_query_contract_ui(void *parameters) {
    ethQueryContractUI_t *msg = (ethQueryContractUI_t *) parameters;
    context_t *context = (context_t *) msg->pluginContext;

    memset(msg->title, 0, msg->titleLength);
    memset(msg->msg, 0, msg->msgLength);
    msg->result = ETH_PLUGIN_RESULT_OK;

    screens_t screen = get_screen(msg);

    switch (context->selectorIndex) {
        case DOWNGRADE:
        case DOWNGRADE_TO_ETH:
        case UPGRADE:
            switch (screen) {
                case SEND_SCREEN:
                    set_amount_ui(msg, context);
                    break;
                case RECEIVE_SCREEN:
                    set_receive_ui(msg, context);
                    break;
                default:
                    PRINTF("Received an invalid screenIndex\n");
                    msg->result = ETH_PLUGIN_RESULT_ERROR;
                    return;
            }
            break;
        case CALL_AGREEMENT:
            switch (screen) {
                case SEND_SCREEN:
                    if (set_cfa_from_ui(msg, context)) {
                        msg->result = ETH_PLUGIN_RESULT_ERROR;
                    }
                    break;
                case RECEIVE_SCREEN:
                    set_cfa_to_ui(msg, context);
                    break;
                default:
                    PRINTF("Received an invalid screenIndex\n");
                    msg->result = ETH_PLUGIN_RESULT_ERROR;
                    return;
            }
            break;
        case UPGRADE_TO_ETH:
            switch (screen) {
                case SEND_SCREEN:
                    set_upgrade_to_eth_send_ui(msg, context);
                    break;
                case RECEIVE_SCREEN:
                    set_upgrade_to_eth_received_ui(msg, context);
                    break;
                default:
                    PRINTF("Received an invalid screenIndex\n");
                    msg->result = ETH_PLUGIN_RESULT_ERROR;
                    return;
            }
            break;

        case BATCH_CALL:
            switch (screen) {
                case SEND_SCREEN:
                    if (set_batch_call_from_ui(msg, context)) {
                        msg->result = ETH_PLUGIN_RESULT_ERROR;
                    }
                    break;
                case RECEIVE_SCREEN:
                    set_batch_call_to_ui(msg, context);
                    break;
                default:
                    break;
            }
            break;

        default:
            PRINTF("Missing selectorIndex: %d\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }
}