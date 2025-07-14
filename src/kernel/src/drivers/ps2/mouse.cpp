#include "drivers/ps2/mouse.hpp"
#include "arch/generic.hpp"

void ps2_mouse_handle_interrupt() {
    // TODO @since 14/07/2025 -- 18:59
    in_port<uint8_t>(0x60);
}

void ps2_mouse_write(uint8_t value) {
    out_port<uint8_t>(PS2_MOUSE_CMD_PORT, PS2_MOUSE_WRITE_TO_MOUSE);
    out_port<uint8_t>(PS2_MOUSE_DATA_PORT, value);
    in_port<uint8_t>(PS2_MOUSE_DATA_PORT);
}

void ps2_mouse_init() {
    out_port<uint8_t>(PS2_MOUSE_CMD_PORT, PS2_MOUSE_ENABLE_AUX_DEVICE);

    out_port<uint8_t>(PS2_MOUSE_CMD_PORT, PS2_MOUSE_READ_CMD_BYTE);
    uint8_t status = in_port<uint8_t>(PS2_MOUSE_DATA_PORT);
    status |= PS2_MOUSE_CMD_ENABLE_MOUSE_IRQ;
    out_port<uint8_t>(PS2_MOUSE_CMD_PORT, PS2_MOUSE_WRITE_CMD_BYTE);
    out_port<uint8_t>(PS2_MOUSE_DATA_PORT, status);

    ps2_mouse_write(PS2_MOUSE_MOUSE_SET_DEFAULTS);
    ps2_mouse_write(PS2_MOUSE_MOUSE_ENABLE_REPORTING);

    ps2_mouse_write(PS2_MOUSE_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_MOUSE_SAMPLE_RATE_1);

    ps2_mouse_write(PS2_MOUSE_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_MOUSE_SAMPLE_RATE_2);

    ps2_mouse_write(PS2_MOUSE_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_MOUSE_SAMPLE_RATE_3);
}