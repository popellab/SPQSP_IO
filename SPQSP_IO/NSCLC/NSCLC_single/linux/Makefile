# makefile for SerializationTest
# testing boost library

CPP = g++-4.8
#CPP = g++

# compiler flags:
# -Wall turns on most compiler warning
WARNING = -w
#WARNING = -Wall
CFLAGS = $(WARNING) -std=c++11 -MMD -MP -O3 -DNDEBUG

# linker flags
LFLAGS = -std=c++11

projectDir :=$(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../../) 


boostIncludeDir = $(HOME)/lib/boost_1_70_0/include
cvodeIncludeDir = $(HOME)/lib/sundials-4.0.1/include

boostLibDir = $(HOME)/lib/boost_1_70_0/lib
cvodeLibDir = $(HOME)/lib/sundials-4.0.1/lib

INCLUDES = -I$(boostIncludeDir) -I$(cvodeIncludeDir) -I$(projectDir)
LIBDIR = -L$(boostLibDir) -L$(cvodeLibDir)

staticLibs = -lboost_serialization -lboost_system -lboost_filesystem\
	 -lboost_date_time -lboost_program_options\
	 -lsundials_cvode -lsundials_nvecserial
dynamicLibs = 
LIBS = -Wl,-Bstatic $(staticLibs)  -Wl,-Bdynamic $(dynamicLibs)

# execuatble file
MAIN = nsclc_sim_single

# list of source files for dependency generation
CPPFILES = $(shell find ../ -name "*.cpp")
CPPFILES += $(shell find ../../SP_QSP_NSCLC/ -name "*.cpp")
CPPFILES += $(shell find ../../../BioFVM/ -name "*.cpp")
CPPFILES += $(shell find ../../../SP_QSP_shared/ -name "*.cpp")
CPPFILES:=$(filter-out ../GenericSolidTumor/*, $(CPPFILES))

SRCS = $(CPPFILES)

OBJS = $(SRCS:.cpp=.o)
DEPS = $(SRCS:.cpp=.d)

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

$(MAIN): $(OBJS) 
	$(CPP) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBDIR) $(LIBS)

%.o: %.cpp
	$(CPP) $(CFLAGS) $(INCLUDES) -c $<  -o $@

profile: CFLAGS += -pg 
profile: clean
profile: $(MAIN)

.PHONY: clean

clean:
	$(RM) $(OBJS) $(DEPS) $(MAIN)

-include $(DEPS)
# DO NOT DELETE THIS LINE -- make depend needs it
