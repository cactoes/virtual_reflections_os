#include "io/kterminal.h"
#include "graphics/framebuffer.h"
#include "libstring.h"

static kterminal_t* global_kterminal;

static
void kterm_draw_cell(kterminal_t* term, char ch) {
    if (term->column >= term->columns || term->row >= term->rows)
        return;

    char glyph[2] = { ch, '\0' };
    const i64 x = (i64)(term->viewport_x + term->column * KTERM_CHAR_WIDTH);
    const i64 y = (i64)(term->viewport_y + term->row * KTERM_CHAR_HEIGHT);

    framebuffer_draw_text8x16(x, y, glyph, term->foreground, term->background, true);
}

static
void kterm_scroll(kterminal_t* term) {
    const u64 width = term->viewport_width;
    const u64 height = term->rows * KTERM_CHAR_HEIGHT;

    if (term->rows > 1)
        framebuffer_copy_rect((i64)term->viewport_x, (i64)(term->viewport_y + KTERM_CHAR_HEIGHT), (i64)width, (i64)(height - KTERM_CHAR_HEIGHT), (i64)term->viewport_x, (i64)term->viewport_y);

    if (term->rows > 0) {
        framebuffer_fill_rect((i64)term->viewport_x, (i64)(term->viewport_y + height - KTERM_CHAR_HEIGHT), (i64)width, KTERM_CHAR_HEIGHT, term->background);
        term->row = term->rows - 1;
    } else {
        term->row = 0;
    }
}

static
void kterm_newline(kterminal_t* term) {
    term->column = 0;
    term->row++;

    if (term->row >= term->rows)
        kterm_scroll(term);
}

static
void kterm_put_printable(kterminal_t* term, char ch) {
    kterm_draw_cell(term, ch);
    term->column++;

    if (term->column >= term->columns)
        kterm_newline(term);
}

static
void kterm_put_tab(kterminal_t* term) {
    const u64 spaces = 4 - (term->column % 4);
    for (u64 i = 0; i < spaces; i++)
        kterm_put_printable(term, ' ');
}

static
u32 kterm_ansi_color(u32 index) {
    static const u8 palette[16][3] = {
        {   0,   0,   0 }, { 170,   0,   0 }, {   0, 170,   0 }, { 170,  85,   0 },
        {   0,   0, 170 }, { 170,   0, 170 }, {   0, 170, 170 }, { 170, 170, 170 },
        {  85,  85,  85 }, { 255,  85,  85 }, {  85, 255,  85 }, { 255, 255,  85 },
        {  85,  85, 255 }, { 255,  85, 255 }, {  85, 255, 255 }, { 255, 255, 255 }
    };

    if (index >= 16)
        index = 15;

    return framebuffer_color(palette[index][0], palette[index][1], palette[index][2]);
}

static
void kterm_apply_sgr(kterminal_t* term) {
    for (u8 i = 0; i < term->ansi_param_count; i++) {
        const u32 code = term->ansi_params[i];

        if (code == 0) {
            term->foreground = term->default_foreground;
            term->background = term->default_background;
        } else if (code == 39) {
            term->foreground = term->default_foreground;
        } else if (code == 49) {
            term->background = term->default_background;
        } else if (code >= 30 && code <= 37) {
            term->foreground = kterm_ansi_color(code - 30);
        } else if (code >= 90 && code <= 97) {
            term->foreground = kterm_ansi_color(code - 90 + 8);
        } else if (code >= 40 && code <= 47) {
            term->background = kterm_ansi_color(code - 40);
        } else if (code >= 100 && code <= 107) {
            term->background = kterm_ansi_color(code - 100 + 8);
        }
    }
}

static
void kterm_reset_csi(kterminal_t* term) {
    term->ansi_param_count = 0;
    term->ansi_value = 0;
    term->ansi_have_digit = false;
}

static
void kterm_push_csi_param(kterminal_t* term) {
    if (term->ansi_param_count < 8) {
        term->ansi_params[term->ansi_param_count++] =
            term->ansi_have_digit ? term->ansi_value : 0;
    }

    term->ansi_value = 0;
    term->ansi_have_digit = false;
}

static
void kterm_consume_ansi(kterminal_t* term, u8 ch) {
    if (term->ansi_state == KTERM_ANSI_ESC) {
        if (ch == '[') {
            kterm_reset_csi(term);
            term->ansi_state = KTERM_ANSI_CSI;
        } else {
            term->ansi_state = KTERM_ANSI_TEXT;
        }
        return;
    }

    if (term->ansi_state != KTERM_ANSI_CSI)
        return;

    if (ch >= '0' && ch <= '9') {
        const u32 digit = ch - '0';
        term->ansi_have_digit = true;
        if (term->ansi_value < 10000)
            term->ansi_value = term->ansi_value * 10 + digit;
        if (term->ansi_value > 10000)
            term->ansi_value = 10000;
        return;
    }

    if (ch == ';') {
        kterm_push_csi_param(term);
        return;
    }

    if (ch == 'm') {
        kterm_push_csi_param(term);
        kterm_apply_sgr(term);
        term->ansi_state = KTERM_ANSI_TEXT;
        kterm_reset_csi(term);
        return;
    }

    if (ch >= 0x40 && ch <= 0x7e) {
        term->ansi_state = KTERM_ANSI_TEXT;
        kterm_reset_csi(term);
    }
}

static
bool kterm_write(kterminal_t* term, const char* text, u64 length) {
    if (!term || !term->initialized || (!text && length != 0))
        return false;

    for (u64 i = 0; i < length; i++) {
        const u8 ch = (u8)text[i];

        if (term->ansi_state == KTERM_ANSI_ESC ||
            term->ansi_state == KTERM_ANSI_CSI) {
            kterm_consume_ansi(term, ch);
            continue;
        }

        if (ch == 0x1b) {
            term->ansi_state = KTERM_ANSI_ESC;
            continue;
        }

        switch (ch) {
            case '\n':
                kterm_newline(term);
                break;
            case '\r':
                term->column = 0;
                break;
            case '\t':
                kterm_put_tab(term);
                break;
            case '\b':
                if (term->column > 0) {
                    term->column--;
                    kterm_draw_cell(term, ' ');
                }
                break;
            default:
                if (ch >= 32 && ch <= 126) {
                    kterm_put_printable(term, (char)ch);
                } else if (ch >= 127) {
                    kterm_put_printable(term, '?');
                }
                break;
        }
    }

    framebuffer_present();
    return true;
}

bool kterm_init(kterminal_t* term) {
    const u64 screen_width = framebuffer_width();
    const u64 screen_height = framebuffer_height();

    term->viewport_x = 0;
    term->viewport_y = 0;
    term->viewport_width = ((screen_width / KTERM_VIEWPORT_WIDTH_DIVISOR) / KTERM_CHAR_WIDTH) * KTERM_CHAR_WIDTH;
    term->viewport_height = (screen_height / KTERM_CHAR_HEIGHT) * KTERM_CHAR_HEIGHT;
    term->columns = term->viewport_width / KTERM_CHAR_WIDTH;
    term->rows = term->viewport_height / KTERM_CHAR_HEIGHT;

    if (term->columns == 0 || term->rows == 0)
        return false;

    term->column = 0;
    term->row = 0;
    term->default_foreground = framebuffer_color(255, 255, 255);
    term->default_background = framebuffer_color(0, 0, 0);
    term->foreground = term->default_foreground;
    term->background = term->default_background;
    term->ansi_state = KTERM_ANSI_TEXT;
    term->initialized = true;
    kterm_reset_csi(term);

    return true;
}

bool kterm_print(kterminal_t* term, const char* text) {
    if (!text)
        return false;

    u64 length = strlen(text);

    return kterm_write(term, text, length);
}

void kterm_bind(kterminal_t* term) {
    global_kterminal = term;
}

bool kterm_printf(const char* fmt, ...) {
    char buffer[KTERM_PRINTF_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    u64 length = vsprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (length >= sizeof(buffer))
        length = sizeof(buffer) - 1;

    return kterm_write(global_kterminal, buffer, length);
}