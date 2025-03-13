from ledger_bitcoin import Chain
from ragger.backend import RaisePolicy
from ragger.backend.interface import BackendInterface
import os
from pathlib import Path
from typing import Literal, Union
import pytest

# disable reordering of imports for autopep8
# fmt: off

TESTS_ROOT_DIR = Path(__file__).parent
REPO_ROOT_DIR = Path(__file__).parent.parent

# update pythonpath to include the ragger_bitcoin package
import sys
sys.path.append(str(REPO_ROOT_DIR / "bitcoin_app_base" ))

print(str(REPO_ROOT_DIR / "bitcoin_app_base" ))

from ragger_bitcoin import createRaggerClient, RaggerClient

# fmt: on


###########################
### CONFIGURATION START ###
###########################

# You can configure optional parameters by overriding the value of ragger.configuration.OPTIONAL_CONFIGURATION
# Please refer to ragger/conftest/configuration.py for their descriptions and accepted values

#########################
### CONFIGURATION END ###
#########################

# Pull all features from the base ragger conftest using the overridden configuration
pytest_plugins = ("ragger.conftest.base_conftest", )


@pytest.fixture
def bitcoin_network() -> Union[Literal['main'], Literal['test']]:
    network = os.getenv("BITCOIN_NETWORK", "test")
    if network not in ["main", "test"]:
        raise ValueError(
            f'Invalid value for BITCOIN_NETWORK: {network}')
    return network


@pytest.fixture
def client(bitcoin_network: str, backend: BackendInterface) -> RaggerClient:
    if bitcoin_network == "main":
        chain = Chain.MAIN
    elif bitcoin_network == "test":
        chain = Chain.TEST
    else:
        raise ValueError(
            f'Invalid value for BITCOIN_NETWORK: {bitcoin_network}')

    backend.raise_policy = RaisePolicy.RAISE_CUSTOM
    backend.whitelisted_status = [0x9000, 0xE000]
    return createRaggerClient(backend, chain=chain, debug=True, screenshot_dir=TESTS_ROOT_DIR)
