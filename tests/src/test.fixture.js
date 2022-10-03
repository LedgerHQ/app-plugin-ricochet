import Zemu from "@zondax/zemu";
import Eth from "@ledgerhq/hw-app-eth";
import { generate_plugin_config } from "./generate_plugin_config";
import { parseEther, parseUnits, RLP } from "ethers/lib/utils";
import { ethers } from "ethers";

const transactionUploadDelay = 60000;

const sim_options_generic = {
  logging: true,
  startDelay: 10000,
  startText: 'is ready'};

const Resolve = require("path").resolve;

const NANOS_ETH_PATH = Resolve("elfs/ethereum_nanos.elf");
const NANOX_ETH_PATH = Resolve("elfs/ethereum_nanox.elf");

const NANOS_PLUGIN_PATH = Resolve("elfs/plugin_nanos.elf");
const NANOX_PLUGIN_PATH = Resolve("elfs/plugin_nanox.elf");

const NANOS_PLUGIN = { Ricochet: NANOS_PLUGIN_PATH };
const NANOX_PLUGIN = { Ricochet: NANOX_PLUGIN_PATH };



const SPECULOS_ADDRESS = "0xFE984369CE3919AA7BB4F431082D027B4F8ED70C";
const RANDOM_ADDRESS = "0xaaaabbbbccccddddeeeeffffgggghhhhiiiijjjj";
//const FROM_RANDOM_ADDRESS = "0x49c89492c69d81b61615bb5a9b90b40129e2c178";

let genericTx = {
  nonce: Number(0),
  gasLimit: Number(21000),
  gasPrice: parseUnits("1", "gwei"),
  value: parseEther("1"),
  chainId: 137,
  to: RANDOM_ADDRESS,
  //  from: FROM_RANDOM_ADDRESS,
  data: null,
};

const TIMEOUT = 1000000;

/**
 * Generates a serializedTransaction from a rawHexTransaction copy pasted from etherscan.
 * @param {string} rawTx Raw transaction
 * @returns {string} serializedTx
 */
function txFromEtherscan(rawTx) {
  // Remove 0x prefix
  rawTx = rawTx.slice(2);

  let txType = rawTx.slice(0, 2);
  if (txType == "02" || txType == "01") {
    // Remove "02" prefix
    rawTx = rawTx.slice(2);
  } else {
    txType = "";
  }

  let decoded = RLP.decode("0x" + rawTx);
  if (txType != "") {
    decoded = decoded.slice(0, decoded.length - 3); // remove v, r, s
  } else {
    decoded[decoded.length - 1] = "0x"; // empty
    decoded[decoded.length - 2] = "0x"; // empty
    decoded[decoded.length - 3] = "0x01"; // chainID 1
  }

  // Encode back the data, drop the '0x' prefix
  let encoded = RLP.encode(decoded).slice(2);

  // Don't forget to prepend the txtype
  return txType + encoded;
}

/**
 * Emulation of the device using zemu
 * @param {string} device name of the device to emulate (nanos, nanox)
 * @param {function} func
 * @param {boolean} signed the plugin is already signed
 * @returns {Promise}
 */
function zemu(device, func, signed = false, testNetwork) {
  return async () => {
    jest.setTimeout(TIMEOUT);
    let eth_path;
    let plugin;
    let sim_options = sim_options_generic;

    if (device === "nanos") {
      eth_path = NANOS_ETH_PATH;
      plugin = NANOS_PLUGIN;
      sim_options.model = "nanos";
    } else {
      eth_path = NANOX_ETH_PATH;
      plugin = NANOX_PLUGIN;
      sim_options.model = "nanox";
    }

    const sim = new Zemu(eth_path, plugin);

    try {
      await sim.start(sim_options);
      const transport = await sim.getTransport();
      const eth = new Eth(transport);

      if (!signed) {
        eth.setPluginsLoadConfig({
          baseURL: null,
          extraPlugins: generate_plugin_config(testNetwork),
        });
      }
      await func(sim, eth);
    } finally {
      await sim.close();
    }
  };
}


/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processDowngradeTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false, testNetwork) {
  test('[' + device.label + '] Downgrade ' + token, zemu(device.name, async (sim, eth) => {
    //for (var key in contractAddrs) {
    const label = device.name + "_downgrade_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + contractAddrs[token] + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(contractAddrs[token], abi);
    // URL

    // Constants used to create the transaction
    const amount = parseUnits("10", 18);

    const { data } = await contract.populateTransaction['downgrade(uint256)'](amount);

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = contractAddrs[token];
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = parseEther("0.1");

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);

    await tx;
    //}
  }, signed, testNetwork));
}


/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processDowngradeToEthTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false, testNetwork) {
  test('[' + device.label + '] Downgrade ' + token, zemu(device.name, async (sim, eth) => {
    //for (var key in contractAddrs) {
    const label = device.name + "_downgrade_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + contractAddrs[token] + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(contractAddrs[token], abi);
    // URL

    // Constants used to create the transaction
    const amount = parseUnits("312", 18);

    const { data } = await contract.populateTransaction['downgradeToETH(uint256)'](amount);

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = contractAddrs[token];
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = parseEther("0.1");

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);

    await tx;
    //}
  }, signed, testNetwork));
}



/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processUpgradeTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false, testNetwork) {
  test('[' + device.label + '] Upgrade ' + token, zemu(device.name, async (sim, eth) => {
    //for (var key in contractAddrs) {
    const label = device.name + "_upgrade_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + contractAddrs[token] + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(contractAddrs[token], abi);
    // URL

    // Constants used to create the transaction
    const amount = parseUnits("10", 18);

    const { data } = await contract.populateTransaction['upgrade(uint256)'](amount);

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = contractAddrs[token];
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = parseEther("0.1");

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);

    await tx;
    //}
  }, signed, testNetwork));
}

/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processUpgradeByEthTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false, testNetwork) {
  test('[' + device.label + '] Upgrade ' + token, zemu(device.name, async (sim, eth) => {
    //for (var key in contractAddrs) {
    const label = device.name + "_upgrade_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + contractAddrs[token] + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(contractAddrs[token], abi);
    // URL

    // Constants used to create the transaction
    const amount = parseUnits("5.213", 18);

    const { data } = await contract.populateTransaction['upgradeByETH()']();

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = contractAddrs[token];
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = amount;

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);

    await tx;
    //}
  }, signed, testNetwork));
}



/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processStopTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false, testNetwork) {
  test('[' + device.label + '] Stop ' + token, zemu(device.name, async (sim, eth) => {
    //for (var key in contractAddrs) {
    const label = device.name + "_stop_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + `0x3e14dc1b13c488a8d5d310918780c983bd5982e7` + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(`0x3e14dc1b13c488a8d5d310918780c983bd5982e7`, abi);
    // URL

    // Constants used to create the transaction
    const amount = "0";
    const data = contractAddrs[token];

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = "0x3e14dc1b13c488a8d5d310918780c983bd5982e7";
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = parseEther(amount);

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);

    await tx;
    //}
  }, signed, testNetwork));
}


/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processStartTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false, testNetwork) {
  test('[' + device.label + '] Start ' + token, zemu(device.name, async (sim, eth) => {

    const address = `0x3e14dc1b13c488a8d5d310918780c983bd5982e7`;

    const label = device.name + "_start_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + address + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(address, abi);
    // URL

    // Constants used to create the transaction
    const amount = "0";
    const data = contractAddrs[token];

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = address;
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = parseEther(amount);

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);
    await tx;
  }, signed,testNetwork));
}


/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processEditTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false, testNetwork) {
  test('[' + device.label + '] Edit ' + token, zemu(device.name, async (sim, eth) => {

    const address = `0x3e14dc1b13c488a8d5d310918780c983bd5982e7`;

    const label = device.name + "_edit_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + address + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(address, abi);
    // URL

    // Constants used to create the transaction
    const amount = "0";
    const data = contractAddrs[token];

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = address;
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = parseEther(amount);

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);
    await tx;
  }, signed, testNetwork));
}


/**
 * Function to execute test with the simulator
 * @param {Object} device Device including its name, its label, and the number of steps to process the use case
 * @param {string} transactionUploadDelay transaction upload delay
 * @param {string} pluginName Name of the plugin
 * @param {array} contractAddrs contracts address
 * @param {boolean} signed The plugin is already signed and existing in Ledger database
 */
function processSecondStartTest(device, pluginName, transactionUploadDelay, token, contractAddrs, signed = false,testNetwork) {
  test('[' + device.label + '] Second Start ' + token, zemu(device.name, async (sim, eth) => {

    const address = `0x3e14dc1b13c488a8d5d310918780c983bd5982e7`;

    const label = device.name + "_second_start_" + token;
    const abi_path = `../networks/${testNetwork}/${pluginName}/abis/` + address + '.abi.json';
    const abi = require(abi_path);
    const contract = new ethers.Contract(address, abi);
    // URL

    // Constants used to create the transaction
    const amount = "0";
    const data = contractAddrs[token];

    // Get the generic transaction template
    let unsignedTx = genericTx;
    // Modify `to` to make it interact with the contract
    unsignedTx.to = address;
    // Modify the attached data
    unsignedTx.data = data;
    // Modify the number of ETH sent
    unsignedTx.value = parseEther(amount);

    // Create serializedTx and remove the "0x" prefix
    const serializedTx = ethers.utils.serializeTransaction(unsignedTx).slice(2);

    const tx = eth.signTransaction(
      "44'/60'/0'/0/0",
      serializedTx
    );

    await sim.waitUntilScreenIsNot(
      sim.getMainMenuSnapshot(),
      transactionUploadDelay
    );
    const steps = device.steps
    await sim.navigateAndCompareSnapshots(".", label, [steps, 0]);

    await tx;
  }, signed, testNetwork));
}

module.exports = {
  // processTest,
  processDowngradeTest,
  processDowngradeToEthTest,
  processUpgradeTest,
  processUpgradeByEthTest,
  processStopTest,
  processStartTest,
  processSecondStartTest,
  processEditTest,
  zemu,
  genericTx
};
