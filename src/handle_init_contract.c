#include "ricochet_plugin.h"

// Copy amount sent parameter to amount
static void handle_amount_value(const ethPluginInitContract_t *msg, context_t *context) {
    copy_parameter(context->amount,
                   msg->pluginSharedRO->txContent->value.value,
                   sizeof(context->amount));
}

// Called once to init.
void handle_init_contract(ethPluginInitContract_t *msg) {
    if (msg->interfaceVersion != ETH_PLUGIN_INTERFACE_VERSION_LATEST) {
        msg->result = ETH_PLUGIN_RESULT_UNAVAILABLE;
        return;
    }

    if (msg->pluginContextLength < sizeof(context_t)) {
        PRINTF("Plugin parameters structure is bigger than allowed size\n");
        msg->result = ETH_PLUGIN_RESULT_ERROR;
        return;
    }

    context_t *context = (context_t *) msg->pluginContext;
    memset(context, 0, sizeof(*context));
    context->valid = 1;

    uint8_t i;
    for (i = 0; i < NUM_SELECTORS; i++) {
        if (memcmp((uint8_t *) PIC(RICOCHET_SELECTORS[i]), msg->selector, SELECTOR_SIZE) == 0) {
            context->selectorIndex = i;
            break;
        }
    }
    if (i == NUM_SELECTORS) {
        msg->result = ETH_PLUGIN_RESULT_UNAVAILABLE;
        return;
    }

    // Set `next_param` to be the first field we expect to parse.
    switch (context->selectorIndex) {
        case DOWNGRADE:
        case UPGRADE:
        case DOWNGRADE_TO_ETH:
            context->next_param = AMOUNT;
            break;
        case UPGRADE_TO_ETH:
            handle_amount_value(msg, context);
            context->next_param = NONE;
            break;
        case CALL_AGREEMENT:
            context->next_param = AGREEMENT_CLASS;
            break;
        case BATCH_CALL:
            context->next_param = PATH_OFFSET;
            break;
        default:
            PRINTF("Missing selectorIndex: %d\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }

    msg->result = ETH_PLUGIN_RESULT_OK;
}
