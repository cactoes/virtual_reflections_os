#include "drivers/ps2/ps2.hpp"


// TODO @since 21/05/2026 -- 13:50
// convert this to less cpu bound

// TODO @since 21/05/2026 -- 22:29
// you know the drill
#include "arch/amd64/port.hpp"

void ps2_wait_input() {
    while ((amd64_in_port8(PS2_CMD_PORT) & PS2_CTRL_STATUS_OUT_BUF) == 0);
}

void ps2_wait_output() {
    while (amd64_in_port8(PS2_CMD_PORT) & PS2_CTRL_STATUS_IN_BUF);
}

void ps2_write(u8 port, u8 value) {
    // ps2_wait_output();
    amd64_out_port8(port, value);
}

u8 ps2_read(u8 port) {
    // ps2_wait_input();
    return amd64_in_port8(port);
}

bool ps2_port_test_device(ps2_device_type_t device_type) {
    ps2_write(PS2_CMD_PORT, (device_type == ps2_device_type_t::KEYBOARD) ? PS2_CMD_TEST_FIRST_PORT : PS2_CMD_TEST_SECOND_PORT);
    return ps2_read(PS2_DATA_PORT) == PS2_TEST_PASSED;
}