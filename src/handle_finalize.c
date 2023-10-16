#include "ricochet_plugin.h"

void handle_sent_address_lookup(ethPluginFinalize_t *msg, context_t *context) {
    if (!ADDRESS_IS_NETWORK_TOKEN(context->contract_address_sent)) {
        msg->tokenLookup1 = context->contract_address_sent;
        PRINTF("Setting address sent to: %.*H\n", ADDRESS_LENGTH, context->contract_address_sent);
    } else {
        msg->tokenLookup1 = NULL;
    }
}

void handle_receive_address_lookup(ethPluginFinalize_t *msg, context_t *context) {
    if (!ADDRESS_IS_NETWORK_TOKEN(context->contract_address_received)) {
        msg->tokenLookup2 = context->contract_address_received;
        PRINTF("Setting address received to: %.*H\n",
               ADDRESS_LENGTH,
               context->contract_address_received);
    } else {
        msg->tokenLookup2 = NULL;
    }
}

void handle_finalize(ethPluginFinalize_t *msg) {
    context_t *context = (context_t *) msg->pluginContext;

    if (context->valid) {
        msg->numScreens = 2;

        switch (context->selectorIndex) {
            case DOWNGRADE:
            case DOWNGRADE_TO_ETH:
                handle_receive_address_lookup(msg, context);
                break;
            case UPGRADE:
            case UPGRADE_TO_ETH:
            case CALL_AGREEMENT:
                handle_sent_address_lookup(msg, context);
                break;
            case BATCH_CALL:
                handle_sent_address_lookup(msg, context);
                handle_receive_address_lookup(msg, context);
                break;
            default:
                PRINTF("Missing selectorIndex: %d\n", context->selectorIndex);
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
        }

        msg->uiType = ETH_UI_TYPE_GENERIC;
        msg->result = ETH_PLUGIN_RESULT_OK;
    } else {
        PRINTF("Context not valid\n");
        msg->result = ETH_PLUGIN_RESULT_FALLBACK;
    }
}
