OS=ubuntu
ifeq ($(OS), ubuntu)
TOOLCHAIN=arm-linux-gnueabihf
CC=$(TOOLCHAIN)-gcc
endif

ifeq ($(OS), fedora)
TOOLCHAIN=
CC=gcc
endif

INCLUDE = -I/usr/include/$(TOOLCHAIN)/artik/base -I./include -I../dm_utils/include/ -I../wifi/include -I../lte/include -I/usr/include/ -I/usr/include/$(TOOLCHAIN)/artik/wifi -I/usr/include/$(TOOLCHAIN)/artik/connectivity


LIBRARIES = -lpthread -lhpg-dm-wifi -lquectel-CM -lhpg-dm-utils -lhpg-dm-lte -lartik-sdk-wifi -lartik-sdk-base -lhpg-debug -L../
RM = rm -f  # rm command

TARGET_LIB = output/wifi_lte_mgr

SRCS = src/hpg_dm_wifi_lte_mgr.c src/hpg_dm_wifi_lte_mgr_utils.c src/hpg_dm_wifi_lte_ipc_handler.c


CFLAGS = -g -c -Wall -Os $(INCLUDE)

OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
ifneq ($(wildcard ./output),)
	@echo "[output] directory fonud"
else
	@echo "Did not find [output] directory"
	mkdir ./output
endif
	$(CC)  $(OBJS) -o $(TARGET_LIB) $(LIBRARIES)

$(SRCS:.c=.d):%.d:%.c
	$(CC) $(CFLAGS) -MM $< >$@

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)

