#include "ricochet_plugin.h"

void handle_init_cfa_screen(ethQueryContractID_t *msg, const context_t *context) {
    cfa_method_t *cfaMethod = NULL;

    for (uint8_t i = 0; i < NUM_CFA_METHOD_COLLECTION; i++) {
        cfaMethod = (cfa_method_t *) PIC(&CFA_METHOD_COLLECTION[i]);
        if (compare_array(cfaMethod->method, context->method_cfa, SELECTOR_SIZE) == 0) {
            strlcpy(msg->version, (char *) cfaMethod->method_name, msg->versionLength);
            break;
        }
    }
}

void handle_query_contract_id(ethQueryContractID_t *msg) {
    const context_t *context = (context_t *) msg->pluginContext;

    strlcpy(msg->name, PLUGIN_NAME, msg->nameLength);
    switch (context->selectorIndex) {
        case DOWNGRADE:
        case DOWNGRADE_TO_ETH:
            strlcpy(msg->version, "Downgrade", msg->versionLength);
            break;
        case CALL_AGREEMENT:
        case BATCH_CALL:
            handle_init_cfa_screen(msg, context);
            break;
        case UPGRADE:
        case UPGRADE_TO_ETH:
            strlcpy(msg->version, "Upgrade", msg->versionLength);
            break;
        default:
            PRINTF("Selector index: %d not supported\n", context->selectorIndex);
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            return;
    }
    msg->result = ETH_PLUGIN_RESULT_OK;
}
