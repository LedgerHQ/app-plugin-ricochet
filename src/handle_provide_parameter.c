#include "ricochet_plugin.h"

// Copy amount sent parameter to amoun
static void handle_amount(const ethPluginProvideParameter_t *msg, context_t *context) {
    copy_parameter(context->amount, msg->parameter, sizeof(context->amount));
}

static void handle_agreement_class(const ethPluginProvideParameter_t *msg, context_t *context) {
    memset(context->contract_address_sent, 0, sizeof(context->contract_address_sent));
    memcpy(context->contract_address_sent,
           &msg->parameter[PARAMETER_LENGTH - ADDRESS_LENGTH],
           sizeof(context->contract_address_sent));
}

static void handle_method_cfa(const ethPluginProvideParameter_t *msg, context_t *context) {
    memset(context->method_cfa, 0, sizeof(context->method_cfa));
    memcpy(context->method_cfa, &msg->parameter[0], sizeof(context->method_cfa));

    const cfa_method_t *cfaMethod = NULL;

    for (int i = 0; i < NUM_CFA_METHOD_COLLECTION; i++) {
        cfaMethod = (const cfa_method_t *) PIC(&CFA_METHOD_COLLECTION[i]);
        if (compare_array(cfaMethod->method, context->method_cfa, SELECTOR_SIZE) == 0) {
            context->method_id = cfaMethod->method_id;
            break;
        }
    }
}

static void handle_token_first_part(const ethPluginProvideParameter_t *msg, context_t *context) {
    memset(context->token_address, 0, sizeof(context->token_address));
    memcpy(context->token_address,
           &msg->parameter[PARAMETER_LENGTH - ADDRESS_LENGTH + SELECTOR_SIZE],
           sizeof(context->token_address) - SELECTOR_SIZE);
}

static void handle_token_second_part(const ethPluginProvideParameter_t *msg, context_t *context) {
    memcpy(&context->token_address[ADDRESS_LENGTH - SELECTOR_SIZE],
           &msg->parameter[0],
           SELECTOR_SIZE);
}

static void handle_sent_address_first_part(const ethPluginProvideParameter_t *msg,
                                           context_t *context) {
    memset(context->contract_address_sent, 0, sizeof(context->contract_address_sent));
    memcpy(context->contract_address_sent,
           &msg->parameter[PARAMETER_LENGTH - ADDRESS_LENGTH + SELECTOR_SIZE],
           sizeof(context->contract_address_sent) - SELECTOR_SIZE);
}

static void handle_sent_address_second_part(const ethPluginProvideParameter_t *msg,
                                            context_t *context) {
    memcpy(&context->contract_address_sent[ADDRESS_LENGTH - SELECTOR_SIZE],
           &msg->parameter[0],
           SELECTOR_SIZE);
}

static void handle_receive_address_first_part(const ethPluginProvideParameter_t *msg,
                                              context_t *context) {
    memset(context->contract_address_received, 0, sizeof(context->contract_address_received));
    memcpy(context->contract_address_received,
           &msg->parameter[PARAMETER_LENGTH - ADDRESS_LENGTH + SELECTOR_SIZE],
           sizeof(context->contract_address_received) - SELECTOR_SIZE);
}

static void handle_receive_address_second_part(const ethPluginProvideParameter_t *msg,
                                               context_t *context) {
    memcpy(&context->contract_address_received[ADDRESS_LENGTH - SELECTOR_SIZE],
           &msg->parameter[0],
           SELECTOR_SIZE);
}

static void handle_flow_rate_first_part(const ethPluginProvideParameter_t *msg,
                                        context_t *context) {
    memset(context->amount, 0, sizeof(context->amount));
    memcpy(context->amount,
           &msg->parameter[SELECTOR_SIZE],
           sizeof(context->amount) - SELECTOR_SIZE);
}

static void handle_flow_rate_second_part(const ethPluginProvideParameter_t *msg,
                                         context_t *context) {
    memcpy(&context->amount[INT256_LENGTH - SELECTOR_SIZE], &msg->parameter[0], SELECTOR_SIZE);
}

static void handle_call_agreement(ethPluginProvideParameter_t *msg, context_t *context) {
    uint16_t tmp;

    if (context->go_to_offset == 1) {
        if (msg->parameterOffset != context->offset + SELECTOR_SIZE) {
            return;
        }
        context->go_to_offset = 0;
    }

    switch (context->next_param) {
        case AGREEMENT_CLASS:
            handle_agreement_class(msg, context);
            context->next_param = PATH_OFFSET;
            break;
        case PATH_OFFSET:
            if (!U2BE_from_parameter(msg->parameter, &tmp)) {
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            context->offset = tmp;
            context->next_param = PATH_LENGTH;
            context->skip++;
            break;
        case PATH_LENGTH:
            if (!U2BE_from_parameter(msg->parameter, &context->array_len)) {
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            context->offset = msg->parameterOffset - SELECTOR_SIZE + PARAMETER_LENGTH;
            context->next_param = CALL_DATA;
            break;
        case CALL_DATA:
            // Parse Second Level ABI Encoded Input Data

            if (msg->parameterOffset == context->offset + SELECTOR_SIZE) {
                handle_method_cfa(msg, context);
                handle_token_first_part(msg, context);
            } else if (msg->parameterOffset == context->offset + SELECTOR_SIZE + PARAMETER_LENGTH) {
                handle_token_second_part(msg, context);
                handle_sent_address_first_part(msg, context);
            } else if (msg->parameterOffset ==
                       context->offset + SELECTOR_SIZE + 2 * PARAMETER_LENGTH) {
                handle_sent_address_second_part(msg, context);
                handle_flow_rate_first_part(msg, context);

                if (context->method_id == STOP_STREAM) {
                    handle_receive_address_first_part(msg, context);
                }

            } else if (msg->parameterOffset ==
                       context->offset + SELECTOR_SIZE + 3 * PARAMETER_LENGTH) {
                handle_flow_rate_second_part(msg, context);
                if (context->method_id == STOP_STREAM) {
                    handle_receive_address_second_part(msg, context);
                }
                context->next_param = NONE;
                break;
            }

            context->next_param = CALL_DATA;
            break;
        case NONE:
            break;
        default:
            PRINTF("Param not supported\n");
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

void handle_batch_call(ethPluginProvideParameter_t *msg, context_t *context) {
    uint16_t tmp;

    if (context->go_to_offset == 1) {
        if (msg->parameterOffset != context->offset + SELECTOR_SIZE) {
            return;
        }
        context->go_to_offset = 0;
    }

    switch (context->next_param) {
        case PATH_OFFSET:
            if (!U2BE_from_parameter(msg->parameter, &tmp)) {
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            context->offset = tmp;
            context->next_param = PATH_LENGTH;
            break;
        case PATH_LENGTH:
            if (!U2BE_from_parameter(msg->parameter, &context->array_len)) {
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            context->offset =
                msg->parameterOffset - SELECTOR_SIZE + PARAMETER_LENGTH * context->array_len;
            context->go_to_offset = 1;
            context->next_param = CONTRACT_PATH_OFFSET;
            break;
        case CONTRACT_PATH_OFFSET:
            if (!U2BE_from_parameter(msg->parameter, &tmp)) {
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            context->offset = tmp + (PARAMETER_LENGTH * 2);
            context->go_to_offset = 1;
            context->next_param = OPERATION_TYPE;
            break;

        case OPERATION_TYPE:
            context->next_param = TARGET;
            break;
        case TARGET:
            context->next_param = BYTES_ARRAY_LEN;
            // we pass the standard structural fields from secondary abi encoding (non-dynamic)
            context->offset = msg->parameterOffset - SELECTOR_SIZE + 5 * PARAMETER_LENGTH;
            context->go_to_offset = 1;
            break;

        case BYTES_ARRAY_LEN:
            if (!U2BE_from_parameter(msg->parameter, &context->array_len)) {
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                return;
            }
            context->next_param = INPUT_DATA;
            context->offset = msg->parameterOffset - SELECTOR_SIZE + PARAMETER_LENGTH;
            break;
        case INPUT_DATA:
            if (msg->parameterOffset == context->offset + SELECTOR_SIZE) {
                // Handle method &firstpart
                handle_method_cfa(msg, context);
                handle_sent_address_first_part(msg, context);
            } else if (msg->parameterOffset == context->offset + SELECTOR_SIZE + PARAMETER_LENGTH) {
                handle_sent_address_second_part(msg, context);
                handle_receive_address_first_part(msg, context);
            } else if (msg->parameterOffset ==
                       context->offset + SELECTOR_SIZE + 2 * PARAMETER_LENGTH) {
                handle_receive_address_second_part(msg, context);
                handle_flow_rate_first_part(msg, context);
            } else if (msg->parameterOffset ==
                       context->offset + SELECTOR_SIZE + 3 * PARAMETER_LENGTH) {
                handle_flow_rate_second_part(msg, context);
                context->next_param = NONE;
                break;
            }
            context->next_param = INPUT_DATA;
            break;
        case NONE:
            break;
        default:
            PRINTF("Param not supported\n");
            msg->result = ETH_PLUGIN_RESULT_ERROR;
            break;
    }
}

void handle_provide_parameter(void *parameters) {
    ethPluginProvideParameter_t *msg = (ethPluginProvideParameter_t *) parameters;
    context_t *context = (context_t *) msg->pluginContext;

    msg->result = ETH_PLUGIN_RESULT_OK;

    if (context->skip) {
        // Skip this step, and don't forget to decrease skipping counter.
        context->skip--;
    } else {
        switch (context->selectorIndex) {
            case DOWNGRADE:
            case DOWNGRADE_TO_ETH:
            case UPGRADE:
                handle_amount(msg, context);
                break;
            case CALL_AGREEMENT:
                handle_call_agreement(msg, context);
                break;
            case BATCH_CALL:
                handle_batch_call(msg, context);
                break;
            default:
                PRINTF("Selector Index not supported: %d\n", context->selectorIndex);
                msg->result = ETH_PLUGIN_RESULT_ERROR;
                break;
        }
    }
}
