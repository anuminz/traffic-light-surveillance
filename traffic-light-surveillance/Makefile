CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -g -Wall -Wextra -Wpedantic
CXXFLAGS += -Iinclude -Iapp/include -pthread -MMD -MP
BUILD    := build

LIB_SRCS := $(wildcard app/src/*.cpp)
LIB_OBJS := $(patsubst app/src/%.cpp,$(BUILD)/%.o,$(LIB_SRCS))
TEST_SRCS := $(wildcard tests/*.cpp)

all: $(BUILD)/tlightd $(BUILD)/tlightctl

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: app/src/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/tlightd: app/tools/tlightd.cpp $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD)/tlightctl: app/tools/tlightctl.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

$(BUILD)/run_tests: $(TEST_SRCS) $(LIB_OBJS) | $(BUILD)
	$(CXX) $(CXXFLAGS) -Itests $^ -o $@

test: $(BUILD)/run_tests
	./$(BUILD)/run_tests

integration: all
	./scripts/integration_test.sh

driver:
	$(MAKE) -C driver

clean:
	rm -rf $(BUILD)
	-$(MAKE) -C driver clean

-include $(wildcard $(BUILD)/*.d)

.PHONY: all test integration driver clean
