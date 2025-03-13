from ragger.navigator import NavInsID
from ragger.firmware import Firmware
from ragger_bitcoin.ragger_instructions import Instructions

# The instructions in this file instruct ragger and speculos on how to interact with the device
# during the tests.


def sign_psbt_instruction_approve(model: Firmware, save_screenshot: bool = True) -> Instructions:
    """
    Creates the ragger instructions for signing the custom transaction type.

    Parameters:
        model (Firmware): pass the 'firmware' fixture (provided by ragger).
        save_screenshot (bool): Whether to save the snapshots during the interaction (and compare with
                                the expected snapshots).

    Returns:
        Instructions: A configured Instructions instance..
    """

    instructions = Instructions(model)

    if model.name.startswith("nano"):
        instructions.new_request("Continue", save_screenshot=save_screenshot)
        instructions.same_request("Sign", save_screenshot=save_screenshot)
    else:
        instructions.new_request("Review", NavInsID.USE_CASE_REVIEW_TAP, NavInsID.USE_CASE_REVIEW_TAP,
                                 save_screenshot=save_screenshot)
        instructions.same_request("Transaction type", NavInsID.USE_CASE_REVIEW_TAP, NavInsID.USE_CASE_REVIEW_TAP,
                                  save_screenshot=save_screenshot)

        instructions.confirm_transaction(save_screenshot=save_screenshot)
    return instructions
