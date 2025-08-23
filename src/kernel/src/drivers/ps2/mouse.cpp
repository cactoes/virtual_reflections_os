#include "drivers/ps2/mouse.hpp"
#include "drivers/ps2/ps2.hpp"
#include "utils/event.hpp"

static event_manager_t<const ps2_mouse_state_t*> g_mouse_event_manager {};
static ps2_mouse_state_t g_mouse_state {};
static uint8_t g_mouse_packet_buffer[PS2_MOUSE_PACKET_SIZE] {};
static uint8_t g_mouse_packet_index = 0;

void ps2_mouse_sync_packets() {
    for (int i = 1; i < PS2_MOUSE_PACKET_SIZE; ++i) {
        if (g_mouse_packet_buffer[i] & PS2_MOUSE_STATUS_VALID_PKT) {
            for (int j = 0; j < PS2_MOUSE_PACKET_SIZE - i; ++j)
                g_mouse_packet_buffer[j] = g_mouse_packet_buffer[i + j];

            g_mouse_packet_index = PS2_MOUSE_PACKET_SIZE - i;
            return;
        }
    }

    g_mouse_packet_index = 0;
}

cpu_state_t* ps2_mouse_handle_interrupt(cpu_state_t* p_rsp) {
    g_mouse_packet_buffer[g_mouse_packet_index++] = ps2_read(PS2_DATA_PORT);

    if (g_mouse_packet_index < PS2_MOUSE_PACKET_SIZE)
        return p_rsp;

    const uint8_t status = g_mouse_packet_buffer[0];
    const uint8_t dx_raw = g_mouse_packet_buffer[1];
    const uint8_t dy_raw = g_mouse_packet_buffer[2];
    const uint8_t dz_raw = g_mouse_packet_buffer[3];

    bool valid = status & PS2_MOUSE_STATUS_VALID_PKT;

    if ((status & (PS2_MOUSE_STATUS_X_OVERFLOW | PS2_MOUSE_STATUS_Y_OVERFLOW)) &&
        (status & (PS2_MOUSE_STATUS_MB_LEFT | PS2_MOUSE_STATUS_MB_RIGHT | PS2_MOUSE_STATUS_MB_MIDDLE))) {
        valid = false;
    }

    if (!valid) {
        ps2_mouse_sync_packets();
        return p_rsp;
    }

    g_mouse_state.dx = (status & PS2_MOUSE_STATUS_X_SIGN_BIT) ? dx_raw - 256 : dx_raw;
    g_mouse_state.dy = (status & PS2_MOUSE_STATUS_Y_SIGN_BIT) ? dy_raw - 256 : dy_raw;
    g_mouse_state.dy = -g_mouse_state.dy;

    g_mouse_state.ds = (int8_t)dz_raw;

    g_mouse_state.buttons.left   = status & PS2_MOUSE_STATUS_MB_LEFT;
    g_mouse_state.buttons.right  = status & PS2_MOUSE_STATUS_MB_RIGHT;
    g_mouse_state.buttons.middle = status & PS2_MOUSE_STATUS_MB_MIDDLE;

    g_mouse_packet_index = 0;
    g_mouse_event_manager.fire_event(&g_mouse_state);

    return p_rsp;
}

void ps2_mouse_write(uint8_t value) {
    ps2_write(PS2_CMD_PORT, PS2_MOUSE_WRITE_TO_MOUSE);
    ps2_write(PS2_DATA_PORT, value);
    ps2_read(PS2_DATA_PORT);
}

void ps2_mouse_init() {
    ps2_write(PS2_CMD_PORT, PS2_MOUSE_ENABLE_AUX_DEVICE);

    ps2_write(PS2_CMD_PORT, PS2_MOUSE_READ_CMD_BYTE);
    uint8_t status = ps2_read(PS2_DATA_PORT);

    status |= PS2_MOUSE_CMD_ENABLE_MOUSE_IRQ;

    ps2_write(PS2_CMD_PORT, PS2_MOUSE_WRITE_CMD_BYTE);
    ps2_write(PS2_DATA_PORT, status);

    ps2_mouse_write(PS2_MOUSE_MOUSE_SET_DEFAULTS);
    ps2_mouse_write(PS2_MOUSE_MOUSE_ENABLE_REPORTING);

    ps2_mouse_write(PS2_MOUSE_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_MOUSE_SAMPLE_RATE_1);
    ps2_mouse_write(PS2_MOUSE_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_MOUSE_SAMPLE_RATE_2);
    ps2_mouse_write(PS2_MOUSE_CMD_SET_SAMPLE_RATE);
    ps2_mouse_write(PS2_MOUSE_SAMPLE_RATE_3);
}

void ps2_mouse_event_subscribe(void(*p_handler)(const ps2_mouse_state_t*)) {
    g_mouse_event_manager.subscribe(p_handler);
}

const ps2_mouse_state_t* ps2_mouse_get_state() {
    return &g_mouse_state;
}