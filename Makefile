TARGET_NAME := effekt
SRC_DIRS    := ./src
BUILD_DIR   := ./build
BIN_DIR     := ./bin

CXX         := clang

CXXFLAGS    := -g -Wall -Wextra -O2 -std=c++20
LDFLAGS     := -L/usr/local/lib

SRCS_CPP    := $(shell find $(SRC_DIRS) -name "*.cpp")

OBJS        := $(SRCS_CPP:$(SRC_DIRS)/%.cpp=$(BUILD_DIR)/%.o) \
               $(SRCS_C:$(SRC_DIRS)/%.c=$(BUILD_DIR)/%.o)

DEPS        := $(OBJS:.o=.d)

TARGET      := $(BIN_DIR)/$(TARGET_NAME)

.PHONY: all clean format

all: $(TARGET)

run:
	./$(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "Build successful: $@"

$(BUILD_DIR)/%.o: $(SRC_DIRS)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "Cleaned up build artifacts."

format:
	find . -iname "*.hpp" -o -iname "*.cpp" -o -iname "*.h" -o -iname "*.c" | xargs clang-format -i
