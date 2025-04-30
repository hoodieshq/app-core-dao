from ledger_bitcoin import WalletPolicy
from ledger_bitcoin.psbt import PSBT

from ragger.navigator import Navigator
from ragger.firmware import Firmware

from .conftest import RaggerClient
from .instructions import *


def test_sign_stake_tx(navigator: Navigator, firmware: Firmware, client: RaggerClient, test_name: str) -> None:
    wallet = WalletPolicy(
        "",
        "wpkh(@0/**)",
        [
            "[f5acc2fd/84'/1'/0']tpubDCtKfsNyRhULjZ9XMS4VKKtVcPdVDi8MKUbcSD9MJDyjRu1A2ND5MiipozyyspBT9bg8upEp7a8EAgFxNxXn1d7QkdbL52Ty5jiSLcxPt1P",
        ],
    )
    psbt = PSBT()
    psbt.deserialize("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQEBBQECAfsEAgAAAAABAMACAAAAAAEBkteTU5STYpaazD6mm2dBYgUIh1J35DYGPfH2tMV/iEEAAAAAAP////8BgDl6EgAAAAAWABQTR+gqA3tduzjPjEdZ8kKx9cfgmgJIMEUCIQCJ2mCr7T1A+h807JBkjVqj1lbKUoEB7FVqyeQUkbiW4AIgC1q0vsCiDGu2zqgACafrg3XsPsWPIJk6VIeB9iedgEcBIQM90rAt3EwCSzePotxDq2uBMYtEizXhd7qP26TzCQZ8IAAAAAABAR+AOXoSAAAAABYAFBNH6CoDe127OM+MR1nyQrH1x+CaIgYCfLddNLAFxOufYrvyxFfXY46BPnV+/OyPpoZ32VC2NmIY9azC/VQAAIABAACAAAAAgAAAAAAAAAAAAQ4g+tXQt6sxuPtHpWZY8En2c8OATtJN2KKxR6oZk+bvLvUBDwQAAAAAARAE/f///wABAwgAo+ERAAAAAAEEIgAg2uIp+SyXvQOY3oP3uxjVR//gdKU0sMqrEm3GdzuJDTQAAQMIAAAAAAAAAAABBFNqTFBTQVQrAQRb3mC30Oa3WMpd2MYdN3osXxr1HsGp4gn16gA2yML0EHijzr7lfYpH1QEEH14OZrF1dqkUE0foKgN7Xbs4z4xHWfJCsfXH4JqIrAA=")
    
    sign_results = client.sign_psbt(psbt, wallet, None,
                                    navigator=navigator,
                                    instructions=sign_psbt_instruction_approve(
                                        firmware),
                                    testname=test_name)

    assert len(sign_results) == 1

    signatures = list(sorted(sign_results))

    # Test that the signature is for the correct pubkey
    i_0, psig_0 = signatures[0]
    assert i_0 == 0
    # This is the key derived at m/84'/1'/0'/0/0
    assert psig_0.pubkey == bytes.fromhex(
        "027cb75d34b005c4eb9f62bbf2c457d7638e813e757efcec8fa68677d950b63662")
    
def test_sign_unstake_tx(navigator: Navigator, firmware: Firmware, client: RaggerClient, test_name: str) -> None:
    wallet = WalletPolicy(
        "",
        "wpkh(@0/**)",
        [
            "[f5acc2fd/84'/1'/0']tpubDCtKfsNyRhULjZ9XMS4VKKtVcPdVDi8MKUbcSD9MJDyjRu1A2ND5MiipozyyspBT9bg8upEp7a8EAgFxNxXn1d7QkdbL52Ty5jiSLcxPt1P",
        ],
    )
    psbt = PSBT()
    psbt.deserialize("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQEBBQEBAfsEAgAAAAABAP0nAQEAAAAAAQH61dC3qzG4+0elZljwSfZzw4BO0k3YorFHqhmT5u8u9QAAAAAA/f///wIAo+ERAAAAACIAINriKfksl70DmN6D97sY1Uf/4HSlNLDKqxJtxnc7iQ00AAAAAAAAAABTakxQU0FUKwEEW95gt9Dmt1jKXdjGHTd6LF8a9R7BqeIJ9eoANsjC9BB4o86+5X2KR9UBBB9eDmaxdXapFBNH6CoDe127OM+MR1nyQrH1x+CaiKwCRzBEAiB+jg0MLSnxXcDbof13W8IFHTpm5/+wiTfvPny1T1ZS5AIgXgtvhtl4s8wC2pcTVr9MPQfi1dyF6x5b8aQYuKal3Q8BIQJ8t100sAXE659iu/LEV9djjoE+dX787I+mhnfZULY2YgAAAAABASsAo+ERAAAAACIAINriKfksl70DmN6D97sY1Uf/4HSlNLDKqxJtxnc7iQ00AQQgBB9eDmaxdXapFBNH6CoDe127OM+MR1nyQrH1x+CaiKwBBSAEH14OZrF1dqkUE0foKgN7Xbs4z4xHWfJCsfXH4JqIrCIGAny3XTSwBcTrn2K78sRX12OOgT51fvzsj6aGd9lQtjZiGPWswv1UAACAAQAAgAAAAIAAAAAAAAAAAAEOIC2nPuT61F9PDRV4f9qwMR0OD2gvPJbZo7MelOVNf+WVAQ8EAAAAAAEQBP3///8AIgICcbW3ea2HCDhYd5e89vDHrsWr52pwnXJPSNLibPh08KAY9azC/VQAAIABAACAAAAAgAEAAAAAAAAAAQMIAKPhEQAAAAABBBYAFDXG4N1tPISxa6iF3Kc6yGPQtZPsAA==")
    
    sign_results = client.sign_psbt(psbt, wallet, None,
                                    navigator=navigator,
                                    instructions=sign_psbt_instruction_approve(
                                        firmware),
                                    testname=test_name)

    assert len(sign_results) == 1

    signatures = list(sorted(sign_results))

    # Test that the signature is for the correct pubkey
    i_0, psig_0 = signatures[0]
    assert i_0 == 0
    # This is the key derived at m/84'/1'/0'/0/0
    assert psig_0.pubkey == bytes.fromhex(
        "027cb75d34b005c4eb9f62bbf2c457d7638e813e757efcec8fa68677d950b63662")
  

    
def test_sign_restake_tx(navigator: Navigator, firmware: Firmware, client: RaggerClient, test_name: str) -> None:
    wallet = WalletPolicy(
        "",
        "wpkh(@0/**)",
        [
            "[f5acc2fd/84'/1'/0']tpubDCtKfsNyRhULjZ9XMS4VKKtVcPdVDi8MKUbcSD9MJDyjRu1A2ND5MiipozyyspBT9bg8upEp7a8EAgFxNxXn1d7QkdbL52Ty5jiSLcxPt1P",
        ],
    )
    psbt = PSBT()
    psbt.deserialize("cHNidP8BAgQCAAAAAQMEAAAAAAEEAQEBBQECAfsEAgAAAAABAP0nAQEAAAAAAQH61dC3qzG4+0elZljwSfZzw4BO0k3YorFHqhmT5u8u9QAAAAAA/f///wIAo+ERAAAAACIAINriKfksl70DmN6D97sY1Uf/4HSlNLDKqxJtxnc7iQ00AAAAAAAAAABTakxQU0FUKwEEW95gt9Dmt1jKXdjGHTd6LF8a9R7BqeIJ9eoANsjC9BB4o86+5X2KR9UBBB9eDmaxdXapFBNH6CoDe127OM+MR1nyQrH1x+CaiKwCRzBEAiB+jg0MLSnxXcDbof13W8IFHTpm5/+wiTfvPny1T1ZS5AIgXgtvhtl4s8wC2pcTVr9MPQfi1dyF6x5b8aQYuKal3Q8BIQJ8t100sAXE659iu/LEV9djjoE+dX787I+mhnfZULY2YgAAAAABASsAo+ERAAAAACIAINriKfksl70DmN6D97sY1Uf/4HSlNLDKqxJtxnc7iQ00AQQgBB9eDmaxdXapFBNH6CoDe127OM+MR1nyQrH1x+CaiKwBBSAEH14OZrF1dqkUE0foKgN7Xbs4z4xHWfJCsfXH4JqIrCIGAny3XTSwBcTrn2K78sRX12OOgT51fvzsj6aGd9lQtjZiGPWswv1UAACAAQAAgAAAAIAAAAAAAAAAAAEOIC2nPuT61F9PDRV4f9qwMR0OD2gvPJbZo7MelOVNf+WVAQ8EAAAAAAEQBP3///8AAQMIAKPhEQAAAAABBCIAINriKfksl70DmN6D97sY1Uf/4HSlNLDKqxJtxnc7iQ00AAEDCAAAAAAAAAAAAQRTakxQU0FUKwEEW95gt9Dmt1jKXdjGHTd6LF8a9R7BqeIJ9eoANsjC9BB4o86+5X2KR9UBBB9eDmaxdXapFBNH6CoDe127OM+MR1nyQrH1x+CaiKwA")
    
    sign_results = client.sign_psbt(psbt, wallet, None,
                                    navigator=navigator,
                                    instructions=sign_psbt_instruction_approve(
                                        firmware),
                                    testname=test_name)

    assert len(sign_results) == 1

    signatures = list(sorted(sign_results))

    # Test that the signature is for the correct pubkey
    i_0, psig_0 = signatures[0]
    assert i_0 == 0
    # This is the key derived at m/84'/1'/0'/0/0
    assert psig_0.pubkey == bytes.fromhex(
        "027cb75d34b005c4eb9f62bbf2c457d7638e813e757efcec8fa68677d950b63662")
