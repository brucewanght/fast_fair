###############################################################################
#
# Generic Makefile for C/C++ Program
#
# Description:
# The makefile searches in <SRCDIRS> directories for the source files
# with extensions specified in <SOURCE_EXT>, then compiles the sources
# and finally produces the <PROGRAM>, the executable file, by linking
# the objectives.
# Usage:
#   $ make           compile and link the program.
#   $ make clean     clean the objectives and dependencies.
#   $ make cleanall  clean the objectives, dependencies and executable.
#   $ make rebuild   rebuild the program. The same as make clean && make all.
#==============================================================================
## Customizing Section: adjust the following if necessary.
# The executable file name, must be specified.
PROGRAM := btree

# Source file directory, at least one path should be specified.
SRCDIRS := ./src

# Object file directory, it's best to specify it.
OBJDIR  := ./src

# The source file types (headers excluded), at least one type should be specified.
# The valid suffixes are among of .c, .C, .cc, .cpp, .CPP, .c++, .cp, or .cxx.
SRCEXTS := .c .cc # C/C++ program

# The compiling flags used only for C.
# If it is a C++ program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C parts.
CFLAGS  := -std=gnu11 -O3
CFLAGS  += -g -flto -rdynamic -mtune=native -march=native -Wall -Wextra -pthread

# The compiling flags used only for C++.
# If it is a C program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C++ parts.
CXXFLAGS := -std=gnu++11 -O3
CXXFLAGS += -g

# The library and the link options ( C and C++ common).
LDFLAGS  := -lrt -lm -lpthread
LDFLAGS  += -ljemalloc

##=============================================================================
## Implict Section: change the following only when necessary.
# The C program compiler. Uncomment it to specify yours explicitly.
CC     = gcc
# The C++ program compiler. Uncomment it to specify yours explicitly.
CXX    = g++
# Uncomment the 2 lines to compile C programs as C++ ones.
#CC     = $(CXX)
#CFLAGS = $(CXXFLAGS)
# The command used to delete file.
RM      = rm -f

##=============================================================================
## Stable Section: usually no need to be changed. But you can add more.
SHELL   = /bin/sh
# Search source files with SRCEXTS extension in each SRCDIRS directory
SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))

# Get object names (*.o) from source files in SOURCES
OBJS    = $(foreach x,$(SRCEXTS), \
		  $(patsubst %$(x),%.o,$(filter %$(x),$(SOURCES))))

# Get dependency file names (*.d) from OBJS
DEPS    = $(patsubst %.o,%$(OBJDIR).d,$(OBJS))

.PHONY : all clean cleanall rebuild
all : $(PROGRAM)

##=============================================================================
# Rules for creating the dependency files (.d).
%.d : %.c
	@$(CC) -MM -MD $(CFLAGS) $<
%.d : %.C
	@$(CXX) -MM -MD $(CXXFLAGS) $<
%.d : %.cc
	@$(CXX) -MM -MD $(CXXFLAGS) $<
%.d : %.cpp
	@$(CXX) -MM -MD $(CXXFLAGS) $<
%.d : %.CPP
	@$(CXX) -MM -MD $(CXXFLAGS) $<
%.d : %.c++
	@$(CXX) -MM -MD $(CXXFLAGS) $<
%.d : %.cp
	@$(CXX) -MM -MD $(CXXFLAGS) $<
%.d : %.cxx
	@$(CXX) -MM -MD $(CXXFLAGS) $<

##=============================================================================
# Generate executable file
$(PROGRAM) : $(OBJS)
ifeq ($(strip $(SRCEXTS)), .c)  # C file
	$(CC) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
else                            # C++ file
	$(CXX) -o $(PROGRAM) $(OBJS) $(LDFLAGS)
endif
# Include dependency files
-include $(DEPS)

rebuild: clean all
clean:
	@$(RM) $(SRCDIRS)/*.o *.d
cleanall: clean
	@$(RM) $(PROGRAM)
###############################################################################
