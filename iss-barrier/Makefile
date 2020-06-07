CC = gcc
COPT = -O3 -march=native -mtune=native
#$(info - Detected -march= $(shell gcc -march=native -mtune=native -Q --help=target | grep -- '-march=' | cut -f3))
#$(info - Detected -mtune= $(shell gcc -march=native -mtune=native -Q --help=target | grep -- '-mtune=' | cut -f3))

# Help detection for ARM SBCs, using devicetree
F_CHECKDTMODEL = $(if $(findstring $(1),$(shell cat /sys/firmware/devicetree/base/model 2>/dev/null)),$(2))
# Jetson Nano is detected correctly
# Raspberry Pi 2 / Zero is detected correctly
DTMODEL_RPI2 = Raspberry Pi 2 Model B 
DTMODEL_RPI3 = Raspberry Pi 3 Model B 
DTMODEL_RPI4 = Raspberry Pi 4 Model B 
COPT_RPI2 = -mfpu=neon-vfpv4
COPT_RPI34 = -mfpu=neon-fp-armv8
COPT += $(call F_CHECKDTMODEL,$(DTMODEL_RPI2),$(COPT_RPI2))
COPT += $(call F_CHECKDTMODEL,$(DTMODEL_RPI3),$(COPT_RPI34))
COPT += $(call F_CHECKDTMODEL,$(DTMODEL_RPI4),$(COPT_RPI34))
# Required for NEON, warning: may lead to loss of floating-point precision
#COPT += -funsafe-math-optimizations

CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=gnu11 -D_GNU_SOURCE
CFLAGS += -D BUILD_VERSION="\"$(shell git describe --dirty --always 2>/dev/null)\""	\
		-D BUILD_DATE="\"$(shell date '+%Y-%m-%d_%H:%M:%S')\""

LIBPREDICT_DIR = libpredict
LIBPREDICT_SRCS = $(LIBPREDICT_DIR)/julian_date.c \
		$(LIBPREDICT_DIR)/observer.c \
		$(LIBPREDICT_DIR)/sun.c \
		$(LIBPREDICT_DIR)/orbit.c \
		$(LIBPREDICT_DIR)/refraction.c \
		$(LIBPREDICT_DIR)/sgp4.c \
		$(LIBPREDICT_DIR)/sdp4.c \
		$(LIBPREDICT_DIR)/unsorted.c

BIN = iss-barrier
SRC = main.c \
	$(LIBPREDICT_SRCS)

LIBSDIR = $(LIBPREDICT_DIR)
LIBS = -lm

all:
	$(CC) $(COPT) $(CFLAGS) $(SRC) -o $(BIN) -I$(LIBSDIR) $(LIBS)

debug: COPT = -Og -ggdb -fno-omit-frame-pointer -D__DEBUG
debug: all

clean:
	rm -fv $(BIN)
