TOP_DIR = ./..
BUILD_DIR = $(TOP_DIR)/build
INCLUDE_DIR = $(TOP_DIR)/src

LDFLAGS =
LDLIBS =

VPATH = $(BUILD_DIR) $(INCLUDE_DIR)

include $(TOP_DIR)/rules.mk

.PHONY: clean

test: build ringBufferTest ttyTest

build:
	@mkdir -p $(BUILD_DIR)

ringBufferTest: ringBufferTest.o
	-cd $(BUILD_DIR) && \
		$(CXX) $(CXXFLAGS) $(^F) -o $@

ttyTest: ttyTest.o tty.o
	-cd $(BUILD_DIR) && \
		$(CXX) $(CXXFLAGS) $(^F) -lpthread -o $@

clean:
	- rm $(BUILD_DIR)/*Test
