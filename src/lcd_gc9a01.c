// Commands for sending messages to a GC9A01 circular TFT display driver
//
// Copyright (C) 2025  Eug Krashtan <eug.krashtan@gmail.com>
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "autoconf.h" // CONFIG_MACH_AVR
#include "basecmd.h" // oid_alloc
#include "board/gpio.h" // gpio_out_write
#include "board/irq.h" // irq_disable
#include "board/misc.h" // timer_from_us
#include "command.h" // DECL_COMMAND
#include "sched.h" // DECL_SHUTDOWN
#include "spicmds.h" // spidev_transfer
#include "generic/armcm_timer.h" // udelay

struct gc9a01 {
    struct spidev_s *spi;
    struct gpio_out dc, reset;
    uint32_t last_cmd_time, cmd_wait_ticks;
};


/****************************************************************
 * GC9A01 Commands
 ****************************************************************/

// Common GC9A01 commands
#define GC9A01_SLPOUT   0x11  // Sleep Out
#define GC9A01_INVON    0x21  // Display Inversion On
#define GC9A01_DISPON   0x29  // Display On
#define GC9A01_CASET    0x2A  // Column Address Set
#define GC9A01_RASET    0x2B  // Row Address Set
#define GC9A01_RAMWR    0x2C  // Memory Write
#define GC9A01_MADCTL   0x36  // Memory Data Access Control
#define GC9A01_COLMOD   0x3A  // Interface Pixel Format


/****************************************************************
 * Timing functions
 ****************************************************************/

static __always_inline uint32_t
nsecs_to_ticks(uint32_t ns)
{
    return timer_from_us(ns * 1000) / 1000000;
}

// ToDo: use existing udelay function from generic/armcm_timer.c
void __attribute__((weak))
udelay(uint32_t us)
{
    if (CONFIG_MACH_AVR)
        // Slower MCUs don't require a delay
        return;

    uint32_t end = timer_read_time() + timer_from_us(us);
    while (timer_is_before(timer_read_time(), end))
        irq_poll();
}


/****************************************************************
 * Transmit functions
 ****************************************************************/

// Send a command byte to the GC9A01
static void
gc9a01_send_cmd(struct gc9a01 *g, uint8_t cmd)
{
    gpio_out_write(g->dc, 0);  // Command mode
    spidev_transfer(g->spi, 1, sizeof(cmd), &cmd);
}

// Send data bytes to the GC9A01
static void
gc9a01_send_data(struct gc9a01 *g, uint8_t len, uint8_t *data)
{
    gpio_out_write(g->dc, 1);  // Data mode
    spidev_transfer(g->spi, 1, len, data);
}

// Send a command followed by data
static void
gc9a01_cmd_with_data(struct gc9a01 *g, uint8_t cmd, uint8_t len, uint8_t *data)
{
    gc9a01_send_cmd(g, cmd);
    if (len > 0)
        gc9a01_send_data(g, len, data);
}

// Hardware reset sequence
static void
gc9a01_hw_reset(struct gc9a01 *g)
{
    gpio_out_write(g->reset, 1);
    udelay(10);
    gpio_out_write(g->reset, 0);
    udelay(10);
    gpio_out_write(g->reset, 1);
    udelay(120000);  // 120ms
}


/****************************************************************
 * Initialization
 ****************************************************************/

static void
gc9a01_init_display(struct gc9a01 *g)
{
    // Hardware reset
    gc9a01_hw_reset(g);

    // Inter-register enable 1
    gc9a01_send_cmd(g, 0xFE);
    gc9a01_send_cmd(g, 0xEF);

    // Set frame rate
    uint8_t b0_data[] = {0xC0};
    gc9a01_cmd_with_data(g, 0xB0, 1, b0_data);

    // Power control
    uint8_t b2_data[] = {0x13};
    gc9a01_cmd_with_data(g, 0xB2, 1, b2_data);

    uint8_t b3_data[] = {0x13};
    gc9a01_cmd_with_data(g, 0xB3, 1, b3_data);

    uint8_t b4_data[] = {0x22};
    gc9a01_cmd_with_data(g, 0xB4, 1, b4_data);

    uint8_t b6_data[] = {0x11, 0x1F};
    gc9a01_cmd_with_data(g, 0xB6, 2, b6_data);

    // Memory Data Access Control (rotation, BGR)
    uint8_t madctl_data[] = {0x08};  // RGB color filter, normal mode
    gc9a01_cmd_with_data(g, GC9A01_MADCTL, 1, madctl_data);

    // Interface Pixel Format - 16bit color
    uint8_t colmod_data[] = {0x55};  // 16-bit/pixel
    gc9a01_cmd_with_data(g, GC9A01_COLMOD, 1, colmod_data);

    // Sleep Out
    gc9a01_send_cmd(g, GC9A01_SLPOUT);
    udelay(120000);  // 120ms

    // Display Inversion On
    gc9a01_send_cmd(g, GC9A01_INVON);

    // Display On
    gc9a01_send_cmd(g, GC9A01_DISPON);
    udelay(20000);  // 20ms
}


/****************************************************************
 * Interface
 ****************************************************************/

void
command_config_gc9a01(uint32_t *args)
{
    struct gc9a01 *g = oid_alloc(args[0], command_config_gc9a01, sizeof(*g));

    g->spi = spidev_oid_lookup(args[1]);
    g->dc = gpio_out_setup(args[2], 0);
    g->reset = gpio_out_setup(args[3], 0);
    g->cmd_wait_ticks = args[4];

    // Initialize the display
    gc9a01_init_display(g);
}
DECL_COMMAND(command_config_gc9a01,
             "config_gc9a01 oid=%c spi_oid=%c dc_pin=%u"
             " reset_pin=%u delay_ticks=%u");

void
command_gc9a01_send_cmds(uint32_t *args)
{
    struct gc9a01 *g = oid_lookup(args[0], command_config_gc9a01);
    uint8_t len = args[1], *cmds = command_decode_ptr(args[2]);

    uint32_t last_cmd_time = g->last_cmd_time;
    uint32_t cmd_wait_ticks = g->cmd_wait_ticks;

    while (len--) {
        uint8_t cmd = *cmds++;
        while (timer_read_time() - last_cmd_time < cmd_wait_ticks)
            irq_poll();
        gc9a01_send_cmd(g, cmd);
        last_cmd_time = timer_read_time();
    }
    g->last_cmd_time = last_cmd_time;
}
DECL_COMMAND(command_gc9a01_send_cmds, "gc9a01_send_cmds oid=%c cmds=%*s");

void
command_gc9a01_send_data(uint32_t *args)
{
    struct gc9a01 *g = oid_lookup(args[0], command_config_gc9a01);
    uint8_t len = args[1], *data = command_decode_ptr(args[2]);

    uint32_t last_cmd_time = g->last_cmd_time;
    uint32_t cmd_wait_ticks = g->cmd_wait_ticks;

    while (timer_read_time() - last_cmd_time < cmd_wait_ticks)
        irq_poll();

    gc9a01_send_data(g, len, data);
    g->last_cmd_time = timer_read_time();
}
DECL_COMMAND(command_gc9a01_send_data, "gc9a01_send_data oid=%c data=%*s");

void
command_gc9a01_set_window(uint32_t *args)
{
    struct gc9a01 *g = oid_lookup(args[0], command_config_gc9a01);
    uint16_t x0 = args[1];
    uint16_t y0 = args[2];
    uint16_t x1 = args[3];
    uint16_t y1 = args[4];

    // Set column address
    uint8_t caset_data[] = {
        (x0 >> 8) & 0xFF, x0 & 0xFF,
        (x1 >> 8) & 0xFF, x1 & 0xFF
    };
    gc9a01_cmd_with_data(g, GC9A01_CASET, 4, caset_data);

    // Set row address
    uint8_t raset_data[] = {
        (y0 >> 8) & 0xFF, y0 & 0xFF,
        (y1 >> 8) & 0xFF, y1 & 0xFF
    };
    gc9a01_cmd_with_data(g, GC9A01_RASET, 4, raset_data);

    // Memory write command
    gc9a01_send_cmd(g, GC9A01_RAMWR);
}
DECL_COMMAND(command_gc9a01_set_window,
             "gc9a01_set_window oid=%c x0=%hu y0=%hu x1=%hu y1=%hu");

void
gc9a01_shutdown(void)
{
    uint8_t i;
    struct gc9a01 *g;
    foreach_oid(i, g, command_config_gc9a01) {
        gpio_out_write(g->dc, 0);
        gpio_out_write(g->reset, 0);
    }
}
DECL_SHUTDOWN(gc9a01_shutdown);
