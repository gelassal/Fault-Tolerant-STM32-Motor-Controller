CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic

PROJECT_ROOT := ../..
FIRMWARE_INC := $(PROJECT_ROOT)/firmware/control_node/Core/Inc
ENCODER_MATH_SRC := $(PROJECT_ROOT)/firmware/control_node/Core/Src/encoder_math.c
BUILD_DIR := build
TARGET := $(BUILD_DIR)/encoder_math_tests.exe

TEST_SOURCES := test_encoder_math.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(TEST_SOURCES) $(ENCODER_MATH_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(FIRMWARE_INC) \
		$(TEST_SOURCES) $(ENCODER_MATH_SRC) -lm -o $(TARGET)

$(BUILD_DIR):
	if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

run: $(TARGET)
	$(TARGET)

clean:
	if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
