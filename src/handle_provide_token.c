#include "ricochet_plugin.h"

void handle_downgrade_tokens(context_t *context) {
    super_token_ticker_t *currentToken = NULL;
    for (uint8_t i = 0; i < NUM_SUPER_TOKEN_COLLECTION; i++) {
        currentToken = (super_token_ticker_t *) PIC(&SUPER_TOKEN_COLLECTION[i]);
        if (memcmp(currentToken->super_token_address,
                   context->contract_address_received,
                   ADDRESS_LENGTH) == 0) {
            strlcpy(context->ticker_sent,
                    (char *) currentToken->ticker_super_token,
                    sizeof(context->ticker_sent));
            strlcpy(context->ticker_received,
                    (char *) currentToken->ticker_token,
                    sizeof(context->ticker_received));
            break;
        }
    }
}

void handle_upgrade_tokens(context_t *context) {
    super_token_ticker_t *currentToken = NULL;
    for (uint8_t i = 0; i < NUM_SUPER_TOKEN_COLLECTION; i++) {
        currentToken = (super_token_ticker_t *) PIC(&SUPER_TOKEN_COLLECTION[i]);
        if (memcmp(currentToken->super_token_address,
                   context->contract_address_received,
                   ADDRESS_LENGTH) == 0) {
            strlcpy(context->ticker_sent,
                    (char *) currentToken->ticker_token,
                    sizeof(context->ticker_sent));
            strlcpy(context->ticker_received,
                    (char *) currentToken->ticker_super_token,
                    sizeof(context->ticker_received));
            break;
        }
    }
}

void handle_cfa_tokens(context_t *context) {
    contract_address_ticker_t *currentContract = NULL;

    for (uint8_t i = 0; i < NUM_CONTRACT_ADDRESS_COLLECTION; i++) {
        currentContract = (contract_address_ticker_t *) PIC(&CONTRACT_ADDRESS_COLLECTION[i]);

        if ((memcmp(currentContract->contract_address,
                    context->contract_address_received,
                    ADDRESS_LENGTH) == 0 &&
             context->method_id == STOP_STREAM) ||
            (memcmp(currentContract->contract_address,
                    context->contract_address_sent,
                    ADDRESS_LENGTH) == 0 &&
             (context->method_id == UPDATE_STREAM || context->method_id == START_STREAM))) {
            strlcpy(context->ticker_sent,
                    (char *) currentContract->ticker_sent,
                    sizeof(context->ticker_sent));
            strlcpy(context->ticker_received,
                    (char *) currentContract->ticker_received,
                    sizeof(context->ticker_received));
            break;
        }
    }
}

void handle_received_address(const ethPluginProvideInfo_t *msg, context_t *context) {
    memset(context->contract_address_received, 0, sizeof(context->contract_address_received));
    memcpy(context->contract_address_received,
           msg->pluginSharedRO->txContent->destination,
           sizeof(context->contract_address_received));
}

void handle_provide_token(ethPluginProvideInfo_t *msg) {
    context_t *context = (context_t *) msg->pluginContext;

    switch (context->selectorIndex) {
        case DOWNGRADE:
        case DOWNGRADE_TO_ETH:
            context->decimals = DEFAULT_DECIMAL;
            handle_received_address(msg, context);
            handle_downgrade_tokens(context);
            break;
        case UPGRADE:
        case UPGRADE_TO_ETH:
            handle_received_address(msg, context);
            handle_upgrade_tokens(context);
            break;
        case CALL_AGREEMENT:
            handle_cfa_tokens(context);
            break;
        case BATCH_CALL:
            break;
        default:
            break;
    }

    msg->result = ETH_PLUGIN_RESULT_OK;
}
