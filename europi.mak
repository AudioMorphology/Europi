# EUROPI MAKE FILE
program_NAME := europi
program_C_SRCS := $(wildcard *.c)
program_C_OBJS := ${program_C_SRCS:.c=.o}
program_OBJS := $(program_C_OBJS) $(program_CXX_OBJS)
# Plasceholders for any include directories, libraries etc 
program_INCLUDE_DIRS :=
program_LIBRARY_DIRS :=
program_LIBRARIES :=
# Pre-processor flags
CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))
.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
    $(LINK.cc) $(program_OBJS) -o $(program_NAME)
	
clean:
    @- $(RM) $(program_NAME)
    @- $(RM) $(program_OBJS)
distclean: clean
	