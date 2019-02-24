# Additional STM32F0 build rules

# Setup the toolchain
CROSS_PREFIX=arm-none-eabi-

dirs-y += src/stm32f0 src/generic
dirs-y += lib/cmsis-stm32f0/source
dirs-y += lib/hal-stm32f0/source

CFLAGS += -mthumb -mcpu=cortex-m0
CFLAGS += -Ilib/cmsis-core -Iout/board
CFLAGS += -Ilib/cmsis-stm32f0/include -Ilib/cmsis-stm32f0 -Ilib/hal-stm32f0/include
CFLAGS += -DSTM32F042x6 -DUSE_HAL_DRIVER

CFLAGS_klipper.elf += -T $(OUT)stm32f0.ld
CFLAGS_klipper.elf += --specs=nano.specs --specs=nosys.specs

# Add source files
src-y += stm32f0/main.c stm32f0/gpio.c stm32f0/can.c
src-y += $(addprefix ../, $(wildcard lib/hal-stm32f0/source/stm32f0xx_*.c))
src-y += generic/crc16_ccitt.c generic/armcm_irq.c generic/armcm_timer.c
src-y += ../lib/cmsis-stm32f0/source/system_stm32f0xx.c
asmsrc-y = ../lib/cmsis-stm32f0/source/startup_stm32f042x6.s

# Add assembler build rules
$(OUT)%.o: %.s $(OUT)autoconf.h $(OUT)board-link
	@echo "  Assembling $@"
	$(Q)$(AS) $< -o $@

$(OUT)klipper.elf: $(patsubst %.s, $(OUT)src/%.o,$(asmsrc-y))

# Build the linker script
target-y := $(OUT)stm32f0.ld $(target-y)

$(OUT)stm32f0.ld: src/stm32f0/stm32f0.ld $(OUT)board-link
	@echo "  Preprocessing $@"
	$(Q)$(CPP) -P -MD -MT $@ -DFLASH_START=$(CONFIG_FLASH_START) $< -o $@

# Binary output file rules
target-y += $(OUT)klipper.bin

$(OUT)klipper.bin: $(OUT)klipper.elf
	@echo "  Creating hex file $@"
	$(Q)$(OBJCOPY) -O binary $< $@

flash: $(OUT)klipper.bin
	@echo ""
	@echo "  The STM32F0 build does not currently support 'make flash'"
	@echo ""
	@echo "  To flash a board, use J-Link or St-Link tool"


