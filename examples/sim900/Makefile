# name of your application
APPLICATION = sim900

FEATURES_REQUIRED = periph_uart
FEATURES_REQUIRED += periph_timer

# If no BOARD is found in the environment, use this default:
BOARD ?= samr21-xpro

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
CFLAGS += -DDEVELHELP

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1


USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += xtimer
USEMODULE += gnrc_ppp
USEMODULE += sim900
USEMODULE += auto_init_gnrc_netif


include $(RIOTBASE)/Makefile.include
