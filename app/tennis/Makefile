# Default Makefile, for host native GNRC-based networking

# name of your application
APPLICATION = tennis
FEATURES_REQUIRED += cpp

# If no BOARD is found in the environment, use this default:
#BOARD ?= esp32-wroom-32
BOARD = adafruit-feather-nrf52840-sense

CFLAGS += "-DCONFIG_IEEE802154_DEFAULT_CHANNEL=23"
CFLAGS += "-DCONFIG_GNRC_IPV6_NIB_SLAAC=1"
CFLAGS += "-DCONFIG_GCOAP_PDU_BUF_SIZE=300"
CFLAGS += "-DCONFIG_GCOAP_REQ_WAITING_MAX=5"
CFLAGS += "-DCONFIG_GCOAP_NON_TIMEOUT_MSEC=300"
CFLAGS += "-DGCOAP_DTLS_EXTRA_STACKSIZE=1024"
CFLAGS += "-DCONFIG_GNRC_NETIF_PKTQ_POOL_SIZE=64"

# IMU stuff
CFLAGS += "-DLSM6DSXX_PARAM_ACC_ODR=LSM6DSXX_DATA_RATE_104HZ"
CFLAGS += "-DLSM6DSXX_PARAM_GYRO_ODR=LSM6DSXX_DATA_RATE_104HZ"
CFLAGS += "-DLSM6DSXX_PARAM_ACC_FS=LSM6DSXX_ACC_FS_16G"
CFLAGS += "-DLSM6DSXX_PARAM_GYRO_FS=LSM6DSXX_GYRO_FS_2000DPS"

#CFLAGS += "-DCONFIG_GCOAP_OBS_REGISTRATIONS_MAX=3"
#CFLAGS += "-DCONFIG_GCOAP_OBS_CLIENTS_MAX=3"
#CFLAGS += "-DCONFIG_GCOAP_OBS_NOTIFIERS_MAX"

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../RIOT

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += netdev_default

# use GNRC by default
LWIP_IPV4 ?= 0
LWIP_IPV6 ?= 0

ifeq (,$(filter 1, $(LWIP_IPV4) $(LWIP_IPV6)))
	USEMODULE += auto_init_gnrc_netif
	# Specify the mandatory networking modules
	USEMODULE += gnrc_ipv6_default
	# Additional networking modules that can be dropped if not needed
	USEMODULE += gnrc_icmpv6_echo
else
	ifeq (1,$(LWIP_IPV4))
		USEMODULE += ipv4_addr

		USEMODULE += lwip_arp
		USEMODULE += lwip_ipv4
		USEMODULE += lwip_dhcp_auto
		CFLAGS += -DETHARP_SUPPORT_STATIC_ENTRIES=1
	endif

	ifeq (1,$(LWIP_IPV6))
		USEMODULE += ipv6_addr

		USEMODULE += lwip_ipv6
		USEMODULE += lwip_ipv6_autoconfig
	endif
endif

USEMODULE += gcoap
USEMODULE += cord_ep_standalone

# Required by gcoap example
USEMODULE += od
USEMODULE += fmt
USEMODULE += netutils
USEMODULE += random
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_cmds_default
USEMODULE += uri_parser
USEMODULE += ps
USEMODULE += saul_default

CFLAGS += -Ofast
CXXFLAGS += -Ofast

# /rtc resource for regular observe notifications
FEATURES_OPTIONAL += periph_rtc

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

# Instead of simulating an Ethernet connection, we can also simulate
# an IEEE 802.15.4 radio using ZEP
USE_ZEP ?= 0

# set the ZEP port for native
ZEP_PORT_BASE ?= 17754
ifeq (1,$(USE_ZEP))
	USEMODULE += socket_zep
endif

# Pass through application configuration to the docker container, if
# BUILD_IN_DOCKER=1 is used:
DOCKER_ENV_VARS += LWIP_IPV4
DOCKER_ENV_VARS += LWIP_IPV6
DOCKER_ENV_VARS += USE_ZEP
DOCKER_ENV_VARS += ZEP_PORT_BASE

include $(RIOTBASE)/Makefile.include

ifneq (,$(filter periph_rtc,$(USEMODULE)))
	USEMODULE += event_periodic
	USEMODULE += event_periodic_callback
	USEMODULE += event_thread
	USEMODULE += ztimer_msec
	USEMODULE += ztimer_sec
endif

USEMODULE += sema
USEMODULE += printf_float
USEMODULE += benchmark
USEMODULE += bmp280_i2c
USEPKG += mpaland-printf

# For now this goes after the inclusion of Makefile.include so Kconfig symbols
# are available. Only set configuration via CFLAGS if Kconfig is not being used
# for this module.
ifndef CONFIG_KCONFIG_MODULE_GCOAP
## Uncomment to redefine port, for example use 61616 for RFC 6282 UDP compression.
#GCOAP_PORT = 5683
#CFLAGS += -DCONFIG_GCOAP_PORT=$(GCOAP_PORT)

## Uncomment to redefine request token length, max 8.
#GCOAP_TOKENLEN = 2
#CFLAGS += -DCONFIG_GCOAP_TOKENLEN=$(GCOAP_TOKENLEN)

# Increase from default for confirmable block2 follow-on requests
GCOAP_RESEND_BUFS_MAX ?= 3
CFLAGS += -DCONFIG_GCOAP_RESEND_BUFS_MAX=$(GCOAP_RESEND_BUFS_MAX)
endif

term_custom:
	$(RIOTBASE)/dist/tools/pyterm/pyterm -p "/dev/$(PORT)" -b "115200" -ln "/tmp/pyterm-haron"

burn:
	# Create UF2
	$(RIOTBASE)/dist/tools/uf2/uf2conv.py -f 0xADA52840 ./bin/adafruit-feather-nrf52840-sense/tennis.hex --base 0x26000 -o ./programm.uf2

	# Create UF2 as ZIP
	adafruit-nrfutil dfu genpkg --dev-type 0x0052 --application ./bin/adafruit-feather-nrf52840-sense/tennis.hex tennis.zip

	# Upload
	adafruit-nrfutil dfu serial --package tennis.zip -p /dev/$(PORT) -b 1152000
	
# Change Channel if different one used
node1:
	@echo "Node 1 selected"
	$(MAKE) clean
	$(MAKE) -j12 all CFLAGS+=" -DMYID=1 "
	$(MAKE) burn PORT=ttyACM0
	$(MAKE) term_custom PORT=ttyACM0

node2:
	@echo "Node 2 selected"
	$(MAKE) clean
	$(MAKE) -j12 all CFLAGS+=" -DMYID=2 "
	$(MAKE) burn PORT=ttyACM1
	$(MAKE) term_custom PORT=ttyACM1

