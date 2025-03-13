# Technical Specification

> **Warning**
This documentation is a template and shall be updated with your own APDUs, if you added any, and with the special types of transactions that the app can recognize and sign.

## About

This documentation describes the APDU messages interface to communicate with the Boilerplate application, and the custom types of transactions that this apps can support with clear signing.

## APDUs

If your derived application adds any APDUs, document them here in the [same format described for app-boilerplate](https://github.com/LedgerHQ/app-boilerplate/blob/master/APP_SPECIFICATION.md).
Otherwise, delete this section and the Status Words section.

## Transaction Types

If your derived app can sign special types of transactions, document in details:
- the custom Scripts used;
- what the PSBT must contain (if appropriate)

Also, document what security checks the derived app performs on the custom inputs.
 