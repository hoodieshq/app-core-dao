## CoreDAO app

This repository implements the CoreDAO protocol for **clear-signing** Bitcoin PSBTs whose structure is not supported by the vanilla Bitcoin app.

### Supported transaction types

| Type | Synonyms | Purpose in CoreDAO | Key characteristics |
|------|----------|--------------------|---------------------|
| **Lock** | “Stake” | Create a time-locked UTXO that represents the staked funds | Internal inputs only; ≤ 3 outputs (locking output ✚ CoreDAO `OP_RETURN` ✚ optional change) |
| **Unlock** | “Unstake” | Spend a previously locked UTXO back to the user after the CLTV expires | Must spend exactly one locking UTXO; produces **one** unlocking output (to a change address) |
| **Restake** | “combined unlock + lock” | Atomically release a matured stake and create a new stake in the same TX | Internal inputs ✚ the spent locking UTXO; obeys *all* lock & unlock rules **except** it must **not** have a change or unlocking output |

#### 1. General
* The dedicated lock output is always derived from the unique BIP-84 path `84h/0h/0h/0/0` (or `84h/0h/0h/0/0` for testnet).
* A PSBT may contain any number of **internal inputs** (regular wallet UTXOs).
* A PSBT may contain any number of **CoreDAO inputs** (previous lock outputs).

#### 2. Lock (stake) transactions
* If the TX contains an `OP_RETURN` output, that output **must** be a valid CoreDAO metadata output **and** the TX **must** also contain a lock output.
* The PSBT may have **at most one** change output.
* If *any* input is a CoreDAO input, the outputs are limited to **change** and **lock** outputs only (i.e., no arbitrary payments).

#### 3. Unlock (unstake) transactions
* If the TX spends a CLTV (time-locked) CoreDAO UTXO, it **must** follow the CoreDAO unlock format.
* Such a TX **must have exactly one** unlocking output (the returned funds).

#### 4. Restake (combined unlock + lock) transactions
* All rules for *both* Lock and Unlock apply **except**:
  * **No change output** is allowed.
  * **No unlocking output** is allowed (the funds are immediately re-locked).

By enforcing these constraints, the app guarantees that every signature produced on-device corresponds to a transaction that is valid under the CoreDAO staking model and cannot be mis-used for unintended transfers.

## Compiling the app

Initialize the submodule with

```
$ git submodule update --init --recursive
```

Compile the app [as usual](https://github.com/LedgerHQ/app-boilerplate#quick-start-guide).
You should be able to launch it using speculos.

## Running the tests

Create a Python virtual environment and install the requirements:

```
$ python -m venv venv
$ source venv/bin/activate
$ pip install -r tests/requirements.txt
```

Launch the test suite; for example, if you compiled the app for Ledger Flex:

```
$ pytest --device=flex
```
