CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic

PROJECT_ROOT := ../..
FIRMWARE_INC := $(PROJECT_ROOT)/firmware/control_node/Core/Inc
CONTROLLER_SRC := $(PROJECT_ROOT)/firmware/control_node/Core/Src/motor_controller.c
BUILD_DIR := build
TARGET := $(BUILD_DIR)/motor_controller_tests.exe

TEST_SOURCES := \
	fake_motor_driver.c \
	test_motor_controller.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(TEST_SOURCES) $(CONTROLLER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(FIRMWARE_INC) -I. \
		$(TEST_SOURCES) $(CONTROLLER_SRC) -o $(TARGET)

$(BUILD_DIR):
	if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

run: $(TARGET)
	$(TARGET)

clean:
	if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
