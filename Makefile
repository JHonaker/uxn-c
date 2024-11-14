TARGET_EXEC := uxn

BUILD_DIR := ./build
SRC_DIRS := src

# Find all the C files we want to compile
SRCS := $(shell find $(SRC_DIRS)  -name '*.c' -or -name '*.s')

# Prepends BUILD_DIR and appends .o to every src file
# As an example, ./your_dir/hello.cpp turns into ./build/./your_dir/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CFLAGS := -Wall -Wextra # -Werror

DEBUG_FLAGS := -g

TEST_DIR := test
TEST_SRCS := $(shell find $(TEST_DIR) -name '*.c' -or -name '*.s')
TEST_OBJS := $(TEST_SRCS:%=$(BUILD_DIR)/%.o)
TEST_SRC_OBJS := $(filter-out $(BUILD_DIR)/$(SRC_DIRS)/main.c.o, $(OBJS))
TEST_EXEC := $(BUILD_DIR)/test/test_runner

.PHONY: all
all: $(BUILD_DIR)/$(TARGET_EXEC)

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

build/test/%: %.c.o
	mkdir -p $(dir $@)
	$(CC) $(TEST_SRC_OBJS) $(TEST_OBJS) -o $@ $(CFLAGS) $(LDFLAGS)

build/test/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean all

.PHONY: test
test: $(TEST_EXEC)
	echo $(TEST_EXEC)
	./$(TEST_EXEC)

$(TEST_EXEC): $(TEST_OBJS) $(TEST_SRC_OBJS)
	mkdir -p $(dir $@)
	$(CC) $(TEST_OBJS) $(TEST_SRC_OBJS) -o $@ $(CFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

.PHONY: run
run: $(BUILD_DIR)/$(TARGET_EXEC)
	./$(BUILD_DIR)/$(TARGET_EXEC)