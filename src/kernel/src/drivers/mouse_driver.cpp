#include "drivers/mouse_driver.hpp"
#include "common.hpp"
#include "cpu.hpp"

static int8_t g_mouse_data[4] {};
static int    g_cycle = 0;

static mouse_state_t* g_mouse_state = nullptr;

void ps2_mouse_write(uint8_t value) {
    cpu_outb(PS2_CMD_PORT, PS2_WRITE_TO_MOUSE);
    cpu_outb(PS2_DATA_PORT, value);
    cpu_inb(PS2_DATA_PORT);
}

void ps2_mouse_init(mouse_state_t* mouse_state) {
    g_mouse_state = mouse_state;

    cpu_outb(PS2_CMD_PORT, PS2_ENABLE_AUX_DEVICE);

    cpu_outb(PS2_CMD_PORT, PS2_READ_CMD_BYTE);
    uint8_t status = cpu_inb(PS2_DATA_PORT);
    status |= PS2_CMD_ENABLE_MOUSE_IRQ;
    cpu_outb(PS2_CMD_PORT, PS2_WRITE_CMD_BYTE);
    cpu_outb(PS2_DATA_PORT, status);

    ps2_mouse_write(PS2_MOUSE_SET_DEFAULTS);
    ps2_mouse_write(PS2_MOUSE_ENABLE_REPORTING);

    ps2_mouse_write(PS2_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_SAMPLE_RATE_1);

    ps2_mouse_write(PS2_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_SAMPLE_RATE_2);

    ps2_mouse_write(PS2_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_SAMPLE_RATE_3);
}

cpu_state_t* mouse_handle_interrupt(uint64_t code, cpu_state_t* rsp) {
    g_mouse_data[g_cycle++] = (int8_t)cpu_inb(PS2_DATA_PORT);

    if (g_cycle == 4) {
        g_cycle = 0;

        g_mouse_state->dx = (int)g_mouse_data[1];
        g_mouse_state->dy = (int)g_mouse_data[2];
        g_mouse_state->ds = (int)g_mouse_data[3]; // idk if this works

        const auto buttons = (int)g_mouse_data[0];

        g_mouse_state->buttons.left = (buttons & 0x01) != 0;
        g_mouse_state->buttons.right = (buttons & 0x02) != 0;
        g_mouse_state->buttons.middle = (buttons & 0x04) != 0;
    }

    return rsp;
}