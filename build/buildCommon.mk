#######################################################################
# Makefile containing build rules common to all subsystems.
#
# James Molloy, 23-Jan-2008
#
# This makefile assumes several things:
#  * The CC,CXX,CFLAGS,CXXFLAGS,AS,ASFLAGS,LDFLAGS,LDADD variables are all set/unset.
#  * The name of the module/binary/library we are building is in the BUILD_NAME variable.
#  * The BUILD variable holds an absolute path to the /build/ directory.
#  * The SRC variable holds a relative path from the /build directory to the /src directory.
#  * All source files to be compiled are in SRCFILES, each being paths relative to the current directory
#    (where Make is being called from) - Normally your subsystem root, where the secondary Makefile is.
#    e.g. SRCFILES=main.cc mysubdir/anotherFile.cc
#  * All directories in which source files are placed are listed in DIRECTORIES, e.g. in the 
#    above example, DIRECTORIES=mysubdir/
#  * The variable CMD_FLAGS contains -D #ifdef defines.
#  * The variable INCLUDES holds the -I paths.
#
# Note that if you're using the top level makefile (as you should be) you only have to deal with
# the second (BUILD_NAME), fifth (SRCFILES) and sixth (DIRECTORIES) items. :-)

-include $(DEPFILES)

# Take the .cc, .c, .s, .S files and make .o files out of them.
OBJS_BARE := $(SRCFILES:.cc=.o)
OBJS_BARE := $(OBJS_BARE:.c=.o)

# Only generate DEPFILES for the c and cc files!
DEPFILES := $(patsubst %.o,%.d,$(OBJS_BARE))

OBJS_BARE := $(OBJS_BARE:.s=.o)
OBJS_BARE := $(OBJS_BARE:.S=.o)

# Create a directory to store the object files.
OBJ_DIR = $(BUILD)/objects-$(BUILD_NAME)

# Add an OBJ_DIR in the front of every object file,
# so we get absolute paths to each object file (for compile stage).
OBJS_OBJ := $(patsubst %.o, $(OBJ_DIR)/%.o, $(OBJS_BARE))

CUR_DIR = $(shell pwd)

# Link.
all: $(OBJ_DIR) $(OBJS_OBJ)
ifndef LOUD
	@echo "[$(BUILD_NAME)]"
else
	@echo "cd $(OBJ_DIR); $(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(BUILD)/built/$(BUILD_NAME) $(OBJS_BARE) $(LDADD)"
endif
	@cd $(OBJ_DIR); $(CXX) $(LDFLAGS) $(CXXFLAGS) -o $(BUILD)/built/$(BUILD_NAME) $(OBJS_BARE) $(LDADD)
	# Alrighty then - I want this text to be in green, but echo doesn't support it. So use perl! Perl FTW!
	@perl -e "print \"\\e[32m*** $(BUILD_NAME) built successfully.\\e[0m\n\";"

$(OBJ_DIR):
	@mkdir $(OBJ_DIR)
	@for NAME in ${DIRECTORIES}; \
	do mkdir -p $(OBJ_DIR)/$$NAME; \
	done

# To compile a .c file to a .o, we need the object file directory to be made and the .c file to exist.
$(OBJ_DIR)%.o: $(CUR_DIR)%.c $(OBJ_DIR)
ifndef LOUD
	@echo "[$(@F)]"
else
	@echo $(CC) $(CFLAGS) -MMD -MP -MT "$*.d $*.t" $(CMD_FLAGS) $(INCLUDES) -o $@ -c $<
endif
	@$(CC) $(CFLAGS) $(CMD_FLAGS) $(INCLUDES) -o $@ -c $<

$(OBJ_DIR)%.o: $(CUR_DIR)%.cc $(OBJ_DIR)
ifndef LOUD
	@echo "[$(@F)]"
else
	@echo $(CXX) $(CXXFLAGS) -MMD -MP -MT "$*.d $*.t" $(CMD_FLAGS) $(INCLUDES) -o $@ -c $<
endif
	@$(CXX) $(CXXFLAGS) $(CMD_FLAGS) $(INCLUDES) -o $@ -c $<

# .S files get run through the C preprocessor.
$(OBJ_DIR)%.o: $(CUR_DIR)%.S $(OBJECTS)
ifndef LOUD
	@echo "[$(@F)]"
else
	@echo "$(CPP) $(CMD_FLAGS) -I$(SRC) -P -x assembler-with-cpp $< tmp.o; $(AS) $(ASFLAGS) -o $@ tmp.o; rm tmp.o"
endif
	@$(CPP) $(CMD_FLAGS) -I$(SRC) -P -x assembler-with-cpp $< tmp.o
	@$(AS) $(ASFLAGS) -o $@ tmp.o
	@rm tmp.o

# .s files don't get run through the preprocessor.
$(OBJ_DIR)%.o: $(CUR_DIR)%.s $(OBJECTS)
ifndef LOUD
	@echo "[$(@F)]"
else
	@echo $(AS) $(ASFLAGS) -o $@ $<
endif
	@$(AS) $(ASFLAGS) -o $@ $<

