# ****************************************************************************
#    Ledger App Bitcoin
#    (c) 2023 Ledger SAS.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
# ****************************************************************************

########################################
#        Mandatory configuration       #
########################################

# Application version
APPVERSION_M = 0
APPVERSION_N = 1
APPVERSION_P = 0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

APPDEVELOPPER="Ledger"
APPCOPYRIGHT="(c) 2025 Ledger"

VARIANT_PARAM = COIN
VARIANT_VALUES = core_dao core_dao_testnet

# simplify for tests
ifndef COIN
COIN=core_dao_testnet
endif

# Enabling DEBUG flag will enable PRINTF and disable optimizations
#DEBUG = 10

APP_DESCRIPTION ="This app enables to\nlock BTC\non CoreDAO."

ifeq ($(COIN),core_dao)
APPNAME ="CoreDAO"
BITCOIN_NETWORK =mainnet
DEFINES += CORE_DAO_MAINNET

else ifeq ($(COIN),core_dao_testnet)
APPNAME ="CoreDAO Testnet"
BITCOIN_NETWORK =testnet
DEFINES += CORE_DAO_TESTNET

else ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(error Unsupported COIN - use $(VARIANT_VALUES))
endif

APP_SOURCE_PATH += bitcoin_app_base/src src

# Application icons following guidelines:
# https://developers.ledger.com/docs/embedded-app/design-requirements/#device-icon
ICON_NANOX = icons/nanox_app_core.gif
ICON_NANOSP = icons/nanox_app_core.gif
ICON_STAX = icons/stax_app_core.gif
ICON_FLEX = icons/flex_app_core.gif

include bitcoin_app_base/Makefile
