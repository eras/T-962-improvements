################################################################################
# Makefile to build the improved T-962 firmware without LPCXpresso
#
# Makes a 'build' directory in the root of the project.
################################################################################
BASE_NAME := T-962-controller

SRC_DIR := ./src/
BUILD_DIR := ./build/
TARGET := $(BUILD_DIR)$(BASE_NAME).axf

vpath %.c $(SRC_DIR)
vpath %.o $(BUILD_DIR)
vpath %.d $(BUILD_DIR)

CC := arm-none-eabi-gcc
RM := rm -rf

# Source files
C_SRCS += $(wildcard $(SRC_DIR)*.c) $(BUILD_DIR)version.c

S_SRCS += $(wildcard $(SRC_DIR)*.s)

OBJS := $(patsubst $(SRC_DIR)%.c,$(BUILD_DIR)%.o,$(C_SRCS)) $(patsubst $(SRC_DIR)%.s,$(BUILD_DIR)%.o,$(S_SRCS))

C_DEPS := $(wildcard *.d)

all: axf

$(BUILD_DIR)version.c: $(BUILD_DIR)tag
	git describe --always --dirty | \
		sed 's/.*/const char* Version_GetGitVersion(void) { return "&"; }/' > $@

# Always regenerate the git version
.PHONY: $(BUILD_DIR)version.c

$(BUILD_DIR)tag:
	mkdir -p $(BUILD_DIR)
	touch $(BUILD_DIR)tag

$(BUILD_DIR)%.o: $(SRC_DIR)%.c $(BUILD_DIR)tag
	$(CC) -std=gnu99 -DNDEBUG -D__NEWLIB__ -Os -g -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -flto -ffat-lto-objects -mcpu=arm7tdmi -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(BUILD_DIR)%.o: $(SRC_DIR)%.s $(BUILD_DIR)tag
	@echo 'Building file: $<'
	$(CC) -c -x assembler-with-cpp -I $(BUILD_DIR) -DNDEBUG -D__NEWLIB__ -mcpu=arm7tdmi -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	$(CC) -nostdlib -Xlinker -Map="$(BUILD_DIR)$(BASE_NAME).map" -Xlinker --gc-sections -mcpu=arm7tdmi -T "$(BASE_NAME).ld" -o "$(TARGET)" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build


clean:
	-$(RM) $(BUILD_DIR)
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "$(TARGET)";
	-arm-none-eabi-objcopy -v -O ihex "$(TARGET)" "$(BUILD_DIR)$(BASE_NAME).hex"
	-@echo ' '

.PHONY: clean dependents
.SECONDARY: post-build

-include ../makefile.targets
