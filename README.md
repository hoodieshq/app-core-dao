## CoreDAO app

This repository implements the CoreDAO protocol for clear signing transactions of a specific format that the bitcoin app does not support.

This application allows 3 types of transaction (stake, unstake, restake):
- Stake transaction contains internal inputs and at most 3 outputs (the locking output, a OP_RETURN output and an optional change)
- Unstake transaction spend a locking UTXO and must have exactly one output (on a change address)
- Restake contain internal inputs and an input spending the locking UTXO and at most 3 outputs (same as the Stake transaction)

This application assumes the lock output is receivable and spendable on a unique path `84h/0h/0h/0/0`

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
