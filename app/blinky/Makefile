# name of your application
APPLICATION = saul_test

# If no BOARD is found in the environment, use this default:
BOARD ?= esp32-wroom-32

# Change Channel if different one used
CFLAGS += "-DCONFIG_IEEE802154_DEFAULT_CHANNEL=23 -DCONFIG_GNRC_IPV6_NIB_SLAAC=1"

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Use a peripheral timer for the delay, if available
FEATURES_OPTIONAL += periph_timer

# Additional modules
USEMODULE += saul_default
USEMODULE += shell_cmds_default
USEMODULE += ps

include $(RIOTBASE)/Makefile.include
