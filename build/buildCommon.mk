#######################################################################
# Makefile containing build rules common to all subsystems.

# Take the .cc, .c, .s, .S files and make .o files out of them.
OBJS = $(SRCFILES:.cc=.o)
OBJS := $(OBJS:.c=.o)
OBJS := $(OBJS:.s=.o)
OBJS := $(OBJS:.S=.o)

# Create a directory to store the object files.
OBJ_DIR = $(BUILD)/objects-$(BUILD_NAME)

all: $(OBJ_DIR)
	echo $(OBJS)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)
