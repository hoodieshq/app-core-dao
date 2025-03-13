from ragger.backend.interface import BackendInterface

# In this test we check that the XOR custom apdu correctly computes the XOR of
# all the input bytes.

# Note: as this test is using a feature not existing in the `ledger_bitcoin` client,
# the test sends directly raw APDUs.


SW_SUCCESS = 0x9000

CLA_APP = 0xE1
INS_CUSTOM_XOR = 128


def test_xor(backend: BackendInterface) -> None:
    data = bytes([1, 2, 3, 4, 5])
    res = backend.exchange(CLA_APP, INS_CUSTOM_XOR, 0, 0, data)

    assert res.status == SW_SUCCESS
    assert res.data == bytes([1 ^ 2 ^ 3 ^ 4 ^ 5])
