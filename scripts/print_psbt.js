const { PsbtV2 } = require('ledger-bitcoin');
const { fromHex } = require('uint8array-tools');
const { fromBase64 } = require('uint8array-tools');
const bitcoin = require('bitcoinjs-lib');

let {
    toHex
  } = require('./utils.js');

const H = 0x80000000;

const unsafeToHex = toHex;
toHex = function (buffer) {
    if (!buffer) {
        return 'undefined';
    }
    return unsafeToHex(buffer);
}

Object.defineProperty(String.prototype, 'capitalize', {
    value: function() {
      return this.charAt(0).toUpperCase() + this.slice(1);
    },
    enumerable: false
  });

function pathLevelToString(pathLevel) {
    return (pathLevel & H) ? `${pathLevel & ~H}'` : `${pathLevel}`;
}

function safe(f) {
    try {
        return f();
    } catch (e) {
        return 'undefined';
    }
}

function witnessUtxoToString(witnessUtxo) {
    return (`\n    Amount: ${witnessUtxo.amount}\n    Script: ${toHex(witnessUtxo.scriptPubKey)}`);
}

function printBip32Derivations(psbt, in_out, prefix, index) {
    const funcName = `get${in_out.capitalize()}Bip32Derivation`;
    const mapName = `${in_out}Maps`;
    let firstDerivation = true;

    const map = psbt[mapName][index];
    if (!map) {
        return;
    }
    for (const pubkey of map.keys()) {
        if (!pubkey.startsWith(prefix))
            continue;
        if (firstDerivation)
            console.log(`  Bip32 derivations:`);
        console.log(`    Public key: ${pubkey.slice(2)}`);
        let derivation = psbt[funcName](index, fromHex(pubkey.slice(2)));
        if (!derivation)
            continue;
        let path = derivation.path.map(pathLevelToString).join('/');
        console.log(`    Path: [${toHex(derivation.masterFingerprint)}]${path}`);
        firstDerivation = false;
    }
}

function printInputPartialSigs(psbt, index) {
    console.log('  Partial signatures: (not supported by the printer)');
}

function printPSBT(psbt, network) {
    
    // getInputPartialSig(inputIndex: number, pubkey: Buffer): Buffer | undefined;
   
    // getInputTapKeySig(inputIndex: number): Buffer | undefined;
    // getInputTapBip32Derivation(inputIndex: number, pubkey: Buffer): {
    
    // getOutputRedeemScript(outputIndex: number): Buffer;
    // getOutputBip32Derivation(outputIndex: number, pubkey: Buffer): {
    // getOutputAmount(outputIndex: number): number;
    // getOutputScript(outputIndex: number): Buffer;
    // getOutputTapBip32Derivation(outputIndex: number, pubkey: Buffer): {

    console.log('Global');
    console.log(`  Tx version: ${psbt.getGlobalTxVersion()}`);
    console.log(`  Locktime: ${psbt.getGlobalFallbackLocktime()}`);
    console.log(`  Input count: ${psbt.getGlobalInputCount()}`);
    console.log(`  Output count: ${psbt.getGlobalOutputCount()}`);
    console.log(`  PSBT version: ${psbt.getGlobalPsbtVersion()}`);
    console.log(`  Modifiable: ${toHex(psbt.getGlobalTxModifiable())}`);

    console.log();
    console.log('Inputs');
    for (let i = 0; i < psbt.getGlobalInputCount(); i++) {
        console.log(`  Input ${i}`);
        console.log(`  Previous txid: ${toHex(psbt.getInputPreviousTxid(i))}`);
        console.log(`  Output index: ${psbt.getInputOutputIndex(i)}`);
        console.log(`  Sequence: ${psbt.getInputSequence(i)}`);
        console.log(`  Sighash type: ${psbt.getInputSighashType(i)}`);
        console.log(`  Final scriptsig: ${toHex(psbt.getInputFinalScriptsig(i))}`);
        console.log(`  Final witness: ${safe(() => toHex(psbt.getInputFinalScriptwitness(i)))}`);
        console.log(`  Non-witness UTXO: ${safe(() => toHex(psbt.getInputNonWitnessUtxo(i)))}`);
        console.log(`  Witness UTXO: ${witnessUtxoToString(psbt.getInputWitnessUtxo(i))}`);
        console.log(`  Redeem script: ${safe(() => toHex(psbt.getInputRedeemScript(i)))}`);
        printBip32Derivations(psbt, 'input', '06', i);
        printBip32Derivations(psbt, 'input', '16', i);
        console.log(`  Witness script: ${safe(() => toHex(psbt.getInputWitnessScript(i)))}`);
        console.log(`  Final scriptsig: ${safe(() => toHex(psbt.getInputFinalScriptsig(i)))}`);
        console.log(`  Final witness: ${safe(() => toHex(psbt.getInputFinalScriptwitness(i)))}`);
        printInputPartialSigs(psbt, i);

        console.log();
    }

    console.log('Outputs');
    for (let i = 0; i < psbt.getGlobalOutputCount(); i++) {
        console.log(`  Output ${i}`);
        console.log(`  Amount: ${psbt.getOutputAmount(i)}`);
        console.log(`  Script: ${toHex(psbt.getOutputScript(i))}`);
        printBip32Derivations(psbt, 'output', '02', i);
        console.log();
    }
}


if (require.main === module) {
    if (process.argv.length < 3) {
      console.error('Usage: node print_psbt.js base64');
      process.exit(1);
    }
    try {
      psbt = new PsbtV2()
      psbt.deserialize(Buffer.copyBytesFrom(fromBase64(process.argv[2])))
      printPSBT(psbt, bitcoin.networks.testnet);
    } catch (error) {
      console.error('Error:', error);
    }
  }

module.exports = {
    printPSBT
};