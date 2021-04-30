IOP_BIN = hdldsvr.irx
IOP_OBJS = hdldsvr.o imports.o exports.o lwNBD/nbd_server.o

IOP_CFLAGS += -DPS2SDK
IOP_INCS += -Iinclude -I../../iopcore/common
#-Wno-unused -Wno-uninitialized 

ifeq ($(DEBUG),1)
IOP_CFLAGS += -DDEBUG -DNBD_DEBUG
endif

include $(PS2SDK)/Defs.make
include ../../Rules.bin.make
include ../../Rules.make
