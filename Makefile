CC = clang
CXX = g++

CFLAGS := $(CFLAGS) -g -O3 -Wall -Wextra -pedantic -Werror -std=c18 -pthread
CXX_TEST_FLAGS := $(CXX_TEST_FLAGS) -g -O3 -Wall -Wextra -pedantic -std=c++20 -pthread
CXXFLAGS := $(CXX_TEST_FLAGS) -Werror 

BUILDDIR = build
BUILD_TEST_DIR = build/unit_tests

SRCS = main.cpp engine.cpp io.cpp order.cpp order_book.cpp
TEST_SRCS = atomic_map_test.cpp

all: engine client test mygrader

engine: $(SRCS:%=$(BUILDDIR)/%.o)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $(BUILDDIR)/$@

client: $(BUILDDIR)/client.cpp.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $(BUILDDIR)/$@

test: $(TEST_SRCS:%.cpp=$(BUILD_TEST_DIR)/%)

mygrader: $(BUILDDIR)/mygrader.cpp.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $(BUILDDIR)/$@

# Rule to link each test executable
$(BUILD_TEST_DIR)/%: $(BUILD_TEST_DIR)/%.cpp.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)

DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILDDIR)/deps/$(notdir $<).d
COMPILE.cpp = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE_TEST.cpp = $(CXX) $(DEPFLAGS) $(CXX_TEST_FLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

$(BUILD_TEST_DIR)/%.cpp.o: tests/unit_tests/%.cpp | $(BUILD_TEST_DIR)
	$(COMPILE_TEST.cpp) $(OUTPUT_OPTION) $<

$(BUILDDIR)/%.cpp.o: src/%.cpp | $(BUILDDIR)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(BUILDDIR): ; @mkdir -p $@

$(BUILD_TEST_DIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%=$(BUILDDIR)/src/%.d) $(BUILDDIR)/src/client.cpp.d $(BUILDDIR)/src/mygrader.cpp.d

.INTERMEDIATE: $(SRCS:%=$(BUILDDIR)/%.o) $(BUILDDIR)/client.cpp.o $(BUILDDIR)/mygrader.cpp.o

-include $(DEPFILES)
