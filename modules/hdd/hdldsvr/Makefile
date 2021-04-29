IOP_BIN = hdldsvr.irx
IOP_OBJS = hdldsvr.o imports.o exports.o lwNBD/nbd_server.o

IOP_CFLAGS += -Wno-unused -Wno-uninitialized -DPS2SDK
IOP_INCS += -Iinclude -I../../iopcore/common


ifeq ($(DEBUG),1)
IOP_CFLAGS += -DDEBUG
endif

include $(PS2SDK)/Defs.make
include ../../Rules.bin.make
include ../../Rules.make
