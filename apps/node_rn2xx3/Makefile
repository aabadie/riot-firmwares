# name of your application
APPLICATION = node_rn2xx3

# If no BOARD is found in the environment, use this default:
BOARD ?= arduino-zero

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../RIOT

# Application specific definitions and includes
APPLICATION_NAME ?= "RN2483\ LoRa\ Node"

# OTA definitions
USEMODULE += rn2483
USEMODULE += bmp280

include $(RIOTBASE)/Makefile.include

CFLAGS += -DAPPLICATION_NAME="\"$(APPLICATION_NAME)\""