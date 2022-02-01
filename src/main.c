/*******************************************************************************
 *   Ethereum 2 Deposit Application
 *   (c) 2020 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "os.h"
#include "cx.h"

#include "ricochet_plugin.h"

// Function: upgrade(uint256 amount)
// Selector: 0x45977d03
static const uint8_t UPGRADE_SELECTOR[SELECTOR_SIZE] = {0x45, 0x97, 0x7d, 0x03};

// Function: upgradeToETH()
// Selector: 0xcf81464b
static const uint8_t UPGRADE_TO_ETH_SELECTOR[SELECTOR_SIZE] = {0xcf, 0x81, 0x46, 0x4b};

// Function: downgrade(uint256 amount)
// Selector: 0x11bcc81e
static const uint8_t DOWNGRADE_SELECTOR[SELECTOR_SIZE] = {0x11, 0xbc, 0xc8, 0x1e};

// Function: downgradeToETH(uint256 amount)
// Selector: 0x160e8be3
static const uint8_t DOWNGRADE_TO_ETH_SELECTOR[SELECTOR_SIZE] = {0x16, 0x0e, 0x8b, 0xe3};

// Function: callAgreement(address agreementClass, bytes callData, bytes userData)
// MethodID: 0x39255d5b
static const uint8_t CALL_AGREEMENT_SELECTOR[SELECTOR_SIZE] = {0x39, 0x25, 0x5d, 0x5b};

// Function: batchCall(address agreementClass, bytes callData, bytes userData)
// MethodID: 0x6ad3ca7d
static const uint8_t BATCH_CALL_SELECTOR[SELECTOR_SIZE] = {0x6a, 0xd3, 0xca, 0x7d};

// Array of all the different ricochet selectors.
const uint8_t *const RICOCHET_SELECTORS[NUM_SELECTORS] = {DOWNGRADE_SELECTOR,
                                                          DOWNGRADE_TO_ETH_SELECTOR,
                                                          CALL_AGREEMENT_SELECTOR,
                                                          UPGRADE_SELECTOR,
                                                          UPGRADE_TO_ETH_SELECTOR,
                                                          BATCH_CALL_SELECTOR};

// Paraswap uses `0xeeeee` as a dummy address to represent ETH.
const uint8_t RICOCHET_ETH_ADDRESS[ADDRESS_LENGTH] = {0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
                                                      0xee, 0xee, 0xee, 0xee, 0xee, 0xee, 0xee,
                                                      0xee, 0xee, 0xee, 0xee, 0xee, 0xee};

const super_token_ticker_t SUPER_TOKEN_COLLECTION[NUM_SUPER_TOKEN_COLLECTION] = {
    {{0x13, 0x05, 0xf6, 0xb6, 0xdf, 0x9d, 0xc4, 0x71, 0x59, 0xd1,
      0x2e, 0xb7, 0xac, 0x28, 0x04, 0xd4, 0xa3, 0x31, 0x73, 0xc2},
     "DAI ",
     "DAIx "},
    {{0xca, 0xa7, 0x34, 0x9c, 0xea, 0x39, 0x0f, 0x89, 0x64, 0x1f,
      0xe3, 0x06, 0xd9, 0x35, 0x91, 0xf8, 0x75, 0x95, 0xdc, 0x1f},
     "USDC ",
     "USDCx "},
    {{0x27, 0xe1, 0xe4, 0xe6, 0xbc, 0x79, 0xd9, 0x30, 0x32, 0xab,
      0xef, 0x01, 0x02, 0x58, 0x11, 0xb7, 0xe4, 0x72, 0x7e, 0x85},
     "WETH ",
     "ETHx "},
    {{0x2c, 0x53, 0x0a, 0xf1, 0xf0, 0x88, 0xb8, 0x36, 0xfa, 0x0d,
      0xca, 0x23, 0xc7, 0xea, 0x50, 0xe6, 0x69, 0x50, 0x8c, 0x4c},
     "MKR ",
     "MKRx "},
    {{0xda, 0xb9, 0x43, 0xc0, 0x3f, 0x9e, 0x84, 0x79, 0x5d, 0xc7,
      0xbf, 0x51, 0xdd, 0xc7, 0x1d, 0xaf, 0x00, 0x33, 0x38, 0x2b},
     "SUSHI ",
     "SUSHIx "},
    {{0xb6, 0x3e, 0x38, 0xd2, 0x1b, 0x31, 0x71, 0x9e, 0x6d, 0xf3,
      0x14, 0xd3, 0xd2, 0xc3, 0x51, 0xdf, 0x0d, 0x4a, 0x91, 0x62},
     "IDLE ",
     "IDLEx "},
    {{0x40, 0x86, 0xeb, 0xf7, 0x52, 0x33, 0xe8, 0x49, 0x2f, 0x1b,
      0xcd, 0xa4, 0x1c, 0x7f, 0x2a, 0x82, 0x88, 0xc2, 0xfb, 0x92},
     "WBTC ",
     "WBTCx "},
    {{0x3a, 0xd7, 0x36, 0x90, 0x4e, 0x9e, 0x65, 0x18, 0x9c, 0x30,
      0x00, 0xc7, 0xdd, 0x2c, 0x8a, 0xc8, 0xbb, 0x7c, 0xd4, 0xe3},
     "MATIC ",
     "MATICx "}};

const cfa_method_t CFA_METHOD_COLLECTION[NUM_CFA_METHOD_COLLECTION] = {
    {{0x62, 0xfc, 0x30, 0x5e}, "Start Stream", START_STREAM},   // 62 fc 30 5e
    {{0x50, 0x20, 0x9a, 0x62}, "Update Steam", UPDATE_STREAM},  // 50 20 9a 62
    {{0xb4, 0xb3, 0x33, 0xc6}, "Stop Stream", STOP_STREAM},     // b4 b3 33 c6
};

const contract_address_ticker_t CONTRACT_ADDRESS_COLLECTION[NUM_CONTRACT_ADDRESS_COLLECTION] = {

    {{0x9b, 0xef, 0x42, 0x7f, 0xa1, 0xff, 0x52, 0x69, 0xb8, 0x24,
      0xee, 0xd9, 0x41, 0x5f, 0x76, 0x22, 0xb8, 0x12, 0x44, 0xf5},
     "DAIx ",
     "ETHx"},
    {{0x0a, 0x70, 0xfb, 0xb4, 0x5b, 0xc8, 0xc7, 0x0f, 0xb9, 0x4d,
      0x86, 0x78, 0xb9, 0x26, 0x86, 0xbb, 0x69, 0xde, 0xa3, 0xc3},
     "ETHx ",
     "DAIx"},
    {{0xe0, 0xa0, 0xec, 0x8d, 0xee, 0x2f, 0x73, 0x94, 0x3a, 0x6b,
      0x73, 0x1a, 0x2e, 0x11, 0x48, 0x49, 0x16, 0xf4, 0x5d, 0x44},
     "USDCx ",
     "WBTCx"},
    {{0x71, 0xf6, 0x49, 0xeb, 0x05, 0xaa, 0x48, 0xcf, 0x8d, 0x92,
      0x32, 0x8d, 0x1c, 0x48, 0x6b, 0x7d, 0x9f, 0xdb, 0xff, 0x6b},
     "WBTCx ",
     "USDCx"},
    {{0x80, 0x82, 0xab, 0x2f, 0x4e, 0x22, 0x0d, 0xad, 0x92, 0x68,
      0x9f, 0x36, 0x82, 0xf3, 0xe7, 0xa4, 0x2b, 0x20, 0x6b, 0x42},
     "USDCx ",
     "ETHx"},
    {{0x39, 0x41, 0xe2, 0xe8, 0x9f, 0x70, 0x47, 0xe0, 0xac, 0x7b,
      0x9c, 0xce, 0x18, 0xfb, 0xe9, 0x09, 0x27, 0xa3, 0x21, 0x00},
     "ETHx ",
     "USDCx"},
    {{0xe0, 0x93, 0xd8, 0xa4, 0x26, 0x9c, 0xe5, 0xc9, 0x1c, 0xd9,
      0x38, 0x9a, 0x06, 0x46, 0xba, 0xda, 0xb2, 0xc8, 0xd9, 0xa3},
     "USDCx ",
     "MATICx"},
    {{0x93, 0xd2, 0xd0, 0x81, 0x2c, 0x98, 0x56, 0x14, 0x1b, 0x08,
      0x0e, 0x9e, 0xf6, 0xe9, 0x7c, 0x7a, 0x7b, 0x34, 0x2d, 0x7f},
     "MATICx ",
     "USDCx"},
    {{0xa1, 0x52, 0x71, 0x5d, 0xf8, 0x00, 0xdb, 0x59, 0x26, 0x59,
      0x89, 0x17, 0xa6, 0xef, 0x37, 0x02, 0x30, 0x8b, 0xcb, 0x7e},
     "DAIx ",
     "MATICx"},
    {{0x25, 0x0e, 0xfb, 0xb9, 0x4d, 0xe6, 0x8d, 0xd1, 0x65, 0xbd,
      0x6c, 0x98, 0xe8, 0x04, 0xe0, 0x81, 0x53, 0xeb, 0x91, 0xc6},
     "MATICx ",
     "DAIx"},
    {{0xc8, 0x95, 0x83, 0xfa, 0x7b, 0x84, 0xd8, 0x1f, 0xe5, 0x4c,
      0x13, 0x39, 0xce, 0x3f, 0xeb, 0x10, 0xde, 0x8b, 0x4c, 0x96},
     "USDCx ",
     "MKRx"},
    {{0xdc, 0x19, 0xed, 0x26, 0xad, 0x3a, 0x54, 0x4e, 0x72, 0x9b,
      0x72, 0xb5, 0x0b, 0x51, 0x8a, 0x23, 0x1c, 0xba, 0xd9, 0xab},
     "MKRx ",
     "USDCx"},
    {{0x47, 0xde, 0x4f, 0xd6, 0x66, 0x37, 0x3c, 0xa4, 0xa7, 0x93,
      0xe2, 0xe0, 0xe7, 0xf9, 0x95, 0xea, 0x7d, 0x3c, 0x9a, 0x29},
     "DAIx ",
     "MKRx"},
    {{0x94, 0xe5, 0xb1, 0x83, 0x09, 0x06, 0x6d, 0xd1, 0xe5, 0xae,
      0x97, 0x62, 0x8a, 0xfc, 0x9d, 0x4d, 0x7e, 0xb5, 0x81, 0x61},
     "MKRx ",
     "DAIx"},
    {{0xbe, 0x79, 0xa6, 0xfd, 0x39, 0xa8, 0xe8, 0xb0, 0xff, 0x7e,
      0x1a, 0xf1, 0xea, 0x6e, 0x26, 0x46, 0x99, 0x68, 0x05, 0x84},
     "USDCx ",
     "IDLEx"},
};

// Function to dispatch calls from the ethereum app.
void dispatch_plugin_calls(int message, void *parameters) {
    switch (message) {
        case ETH_PLUGIN_INIT_CONTRACT:
            handle_init_contract(parameters);
            break;
        case ETH_PLUGIN_PROVIDE_PARAMETER:
            handle_provide_parameter(parameters);
            break;
        case ETH_PLUGIN_PROVIDE_INFO:
            handle_provide_token(parameters);
            break;
        case ETH_PLUGIN_FINALIZE:
            handle_finalize(parameters);
            break;
        case ETH_PLUGIN_QUERY_CONTRACT_ID:
            handle_query_contract_id(parameters);
            break;
        case ETH_PLUGIN_QUERY_CONTRACT_UI:
            handle_query_contract_ui(parameters);
            break;
        default:
            PRINTF("Unhandled message %d\n", message);
            break;
    }
}

// Calls the ethereum app.
void call_app_ethereum() {
    unsigned int libcall_params[3];
    libcall_params[0] = (unsigned int) "Ethereum";
    libcall_params[1] = 0x100;
    libcall_params[2] = RUN_APPLICATION;
    os_lib_call((unsigned int *) &libcall_params);
}

// Weird low-level black magic. No need to edit this.
__attribute__((section(".boot"))) int main(int arg0) {
    // Exit critical section
    __asm volatile("cpsie i");

    // Ensure exception will work as planned
    os_boot();

    // Try catch block. Please read the docs for more information on how to use those!
    BEGIN_TRY {
        TRY {
            // Low-level black magic.
            check_api_level(CX_COMPAT_APILEVEL);

            // Check if we are called from the dashboard.
            if (!arg0) {
                // Called from dashboard, launch Ethereum app
                call_app_ethereum();
                return 0;
            } else {
                // Not called from dashboard: called from the ethereum app!
                unsigned int *args = (unsigned int *) arg0;

                // If `ETH_PLUGIN_CHECK_PRESENCE` is set, this means the caller is just trying to
                // know whether this app exists or not. We can skip `dispatch_plugin_calls`.
                if (args[0] != ETH_PLUGIN_CHECK_PRESENCE) {
                    dispatch_plugin_calls(args[0], (void *) args[1]);
                }

                // Call `os_lib_end`, go back to the ethereum app.
                os_lib_end();
            }
        }
        FINALLY {
        }
    }
    END_TRY;

    // Will not get reached.
    return 0;
}
