from ledger_bitcoin import WalletPolicy
from ledger_bitcoin.psbt import PSBT

from ragger.navigator import Navigator
from ragger.firmware import Firmware

from .conftest import RaggerClient
from .instructions import *


def test_sign_psbt(navigator: Navigator, firmware: Firmware, client: RaggerClient, test_name: str) -> None:
    wallet = WalletPolicy(
        "",
        "tr(@0/**)",
        [
            "[f5acc2fd/86'/1'/0']tpubDDKYE6BREvDsSWMazgHoyQWiJwYaDDYPbCFjYxN3HFXJP5fokeiK4hwK5tTLBNEDBwrDXn8cQ4v9b2xdW62Xr5yxoQdMu1v6c7UDXYVH27U"
        ],
    )
    psbt = PSBT()
    psbt.deserialize("cHNidP8BAJUCAAAAAqG4I9IzbWlLSTTvm25bfeF6BVE9qKKdsCouy8eppv5tAQAAAAD9////FveaMWPsN+g8VMbi6P9s2IOOg17zrcPf1ZYnyUnsJAkAAAAAAP3///8C8qapAAAAAAAiUSALjnSGvDBqCu+3p8AK8EBVQtsazXPuzKgnccz1/l62DwAAAAAAAAAABWoDRk9PAAAAAAABASunhqkAAAAAACJRINj08dGJltthuxyvVCPeJdih7unJUNN+b/oCMBLV5i4NIRYhLqKFalzxEOZqK+nXNTFHk/28s4iyuPE/K2remC569RkA9azC/VYAAIABAACAAAAAgAEAAAAAAAAAARcgIS6ihWpc8RDmaivp1zUxR5P9vLOIsrjxPytq3pguevUAAQErOTAAAAAAAAAiUSCHtA8hlu4BzfGu7dqCwmls1lYlShMPirSpdE1UaM3XBSEWh7QPIZbuAc3xru3agsJpbNZWJUoTD4q0qXRNVGjN1wURAPWswv1WAACAAQAAgGMAAIABFyCHtA8hlu4BzfGu7dqCwmls1lYlShMPirSpdE1UaM3XBQABBSACkIHs5WFqocuZMZ/Eh07+5H8IzrpfYARjbIxDQJpfCiEHApCB7OVhaqHLmTGfxIdO/uR/CM66X2AEY2yMQ0CaXwoZAPWswv1WAACAAQAAgAAAAIABAAAAAgAAAAAA")

    sign_results = client.sign_psbt(psbt, wallet, None,
                                    navigator=navigator,
                                    instructions=sign_psbt_instruction_approve(
                                        firmware),
                                    testname=test_name)

    assert len(sign_results) == 2

    signatures = list(sorted(sign_results))

    # Test that the signature is for the correct pubkey
    i_0, psig_0 = signatures[0]
    assert i_0 == 0
    # This is the key derived at m/86'/1'/0'/1/0, tweaked as per BIP-86
    assert psig_0.pubkey == bytes.fromhex(
        "d8f4f1d18996db61bb1caf5423de25d8a1eee9c950d37e6ffa023012d5e62e0d")

    i_1, psig_1 = signatures[1]
    assert i_1 == 1
    # This is the key derived at m/86'/1'/99'/1/0, and it is NOT tweaked
    assert psig_1.pubkey == bytes.fromhex(
        "87b40f2196ee01cdf1aeedda82c2696cd656254a130f8ab4a9744d5468cdd705")
