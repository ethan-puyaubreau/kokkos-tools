#Variorum Dependancy
VAR_LIB=-L$(VARIORUM_ROOT)/lib
VAR_INC=-I$(VARIORUM_ROOT)/include
LINK_FLAG=-lvariorum

CXX=mpicxx
CXXFLAGS=-O3 -std=c++17 -g
SHARED_CXXFLAGS=-shared -fPIC
all: build/power-profiler.so

MAKEFILE_PATH := $(subst Makefile,,$(abspath $(lastword $(MAKEFILE_LIST))))

CXXFLAGS+=-I${MAKEFILE_PATH}/src -I${MAKEFILE_PATH}/../../common/makefile-only -I${MAKEFILE_PATH}/../all

SOURCES = src/power-profiler.cpp \
          src/core/daemon.cpp \
          src/core/profiler_core.cpp \
          src/core/config.cpp \
          src/core/power_monitor.cpp \
          src/core/timing_manager.cpp \
          src/providers/energy_provider.cpp \
          src/providers/variorum_energy_provider.cpp \
          src/providers/dummy_energy_provider.cpp \
          src/data/timing_data.cpp \
          src/data/correlation.cpp \
          src/output/output_handler.cpp

HEADERS = $(wildcard src/*.hpp) \
          $(wildcard src/core/*.hpp) \
          $(wildcard src/providers/*.hpp) \
          $(wildcard src/data/*.hpp) \
          $(wildcard src/output/*.hpp)

build/power-profiler.so: $(SOURCES) $(HEADERS) | build
	$(CXX) $(SHARED_CXXFLAGS) $(CXXFLAGS) $(VAR_LIB) $(VAR_INC) -o $@ $(SOURCES) $(LINK_FLAG)

build:
	mkdir -p build

clean:
	rm -rf build/*.so
