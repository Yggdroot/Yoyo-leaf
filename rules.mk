.SUFFIXES:
.SUFFIXES: .cpp .c .cc .o

SHELL = /bin/sh

CPP_VERSION = -std=c++14

CXX = g++ $(CPP_VERSION)
CXXFLAGS = -Wall -I$(INCLUDE_DIR)
LDFLAGS =
LDLIBS = -lpthread

SOURCE = $(wildcard *.c) $(wildcard *.cc) $(wildcard *.cpp)
OBJS = $(addsuffix .o,$(basename $(SOURCE)))
DEPS = $(patsubst %.o,%.d,$(OBJS))

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $(BUILD_DIR)/$@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c $< -o $(BUILD_DIR)/$@

%.d: %.cpp
	@$(CXX) -I$(INCLUDE_DIR) -MM $< -MF $(BUILD_DIR)/$@

%.d: %.cc
	@$(CXX) -I$(INCLUDE_DIR) -MM $< -MF $(BUILD_DIR)/$@

%.d: %.c
	@$(CXX) -I$(INCLUDE_DIR) -MM $< -MF $(BUILD_DIR)/$@

