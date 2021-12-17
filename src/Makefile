TOP_DIR = ./..
BUILD_DIR = $(TOP_DIR)/build
INCLUDE_DIR = .
TEST_DIR = $(TOP_DIR)/test

LDFLAGS =
LDLIBS = -lpthread

VPATH = $(BUILD_DIR) $(TOP_DIR)

include $(TOP_DIR)/rules.mk

TARGET = yy
ALIAS = leaf

.PHONY: $(if $(MAKECMDGOALS), $(MAKECMDGOALS), all)

all: CXXFLAGS += -O3
all: goal

debug: CXXFLAGS += -g
debug: LDFLAGS += -rdynamic -fno-omit-frame-pointer -fasynchronous-unwind-tables
debug: goal

asan: CXXFLAGS += -g
asan: LDFLAGS += -fsanitize=address -fno-omit-frame-pointer -fasynchronous-unwind-tables
asan: goal

goal: build $(DEPS) $(TARGET) $(ALIAS)

build:
	@mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJS)
	-cd $(BUILD_DIR) && \
		$(CXX) $(^F) $(LDFLAGS) $(LDLIBS) -o $@

$(ALIAS): $(TARGET)
	-cd $(BUILD_DIR) && \
		ln -sf $(<F) $@

sinclude $(addprefix $(BUILD_DIR)/,$(DEPS))

clean:
	rm -rf $(BUILD_DIR)

test:
	@$(MAKE) -k $@ -C $(TEST_DIR) || exit 1
