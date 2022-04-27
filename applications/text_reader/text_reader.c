#include "text_reader.h"

#include <furi_hal.h>
#include <furi.h>

// Need access to u8g2
#include <gui/gui_i.h>
#include <gui/canvas_i.h>
#include <u8g2_glue.h>

#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable_item_list.h>

#include "view_text_reader.h"

#define TAG "TextReader"

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    ViewTextReader* view_text_reader;
    VariableItemList* variable_item_list;
    Submenu* submenu;

    bool config_bias;
    uint8_t config_contrast;
    uint8_t config_regulation_ratio;
} TextReader;

typedef enum {
    TextReaderViewSubmenu,
    TextReaderViewConfigure,
    TextReaderViewTextReader,
} TextReaderView;

const bool config_bias_value_text_reader[] = {
    true,
    false,
};
const char* const config_bias_text_reader[] = {
    "1/7",
    "1/9",
};

const uint8_t config_regulation_ratio_value_text_reader[] = {
    0b000,
    0b001,
    0b010,
    0b011,
    0b100,
    0b101,
    0b110,
    0b111,
};
const char* const config_regulation_ratio_text_reader[] = {
    "3.0",
    "3.5",
    "4.0",
    "4.5",
    "5.0",
    "5.5",
    "6.0",
    "6.5",
};

static void text_reader_submenu_callback(void* context, uint32_t index) {
    TextReader* instance = (TextReader*)context;
    view_dispatcher_switch_to_view(instance->view_dispatcher, index);
}

static uint32_t text_reader_previous_callback(void* context) {
    return TextReaderViewSubmenu;
}

static uint32_t text_reader_exit_callback(void* context) {
    return VIEW_NONE;
}

static void text_reader_reload_config(TextReader* instance) {
    FURI_LOG_I(
        TAG,
        "contrast: %d, regulation_ratio: %d, bias: %d",
        instance->config_contrast,
        instance->config_regulation_ratio,
        instance->config_bias);
    u8x8_d_st756x_init(
        &instance->gui->canvas->fb.u8x8,
        instance->config_contrast,
        instance->config_regulation_ratio,
        instance->config_bias);
    gui_update(instance->gui);
}

static void display_config_set_bias(VariableItem* item) {
    TextReader* instance = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, config_bias_text_reader[index]);
    instance->config_bias = config_bias_value_text_reader[index];
    text_reader_reload_config(instance);
}

static void display_config_set_regulation_ratio(VariableItem* item) {
    TextReader* instance = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    variable_item_set_current_value_text(item, config_regulation_ratio_text_reader[index]);
    instance->config_regulation_ratio = config_regulation_ratio_value_text_reader[index];
    text_reader_reload_config(instance);
}

static void display_config_set_contrast(VariableItem* item) {
    TextReader* instance = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);
    string_t temp;
    string_init(temp);
    string_cat_printf(temp, "%d", index);
    variable_item_set_current_value_text(item, string_get_cstr(temp));
    string_clear(temp);
    instance->config_contrast = index;
    text_reader_reload_config(instance);
}

TextReader* text_reader_alloc() {
    TextReader* instance = malloc(sizeof(TextReader));

    View* view = NULL;

    instance->gui = furi_record_open("gui");
    instance->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(instance->view_dispatcher);
    view_dispatcher_attach_to_gui(
        instance->view_dispatcher, instance->gui, ViewDispatcherTypeFullscreen);

    // Test
    instance->view_text_reader = view_text_reader_alloc();
    view = view_text_reader_get_view(instance->view_text_reader);
    view_set_previous_callback(view, text_reader_previous_callback);
    view_dispatcher_add_view(instance->view_dispatcher, TextReaderViewTextReader, view);

    // Configure
    instance->variable_item_list = variable_item_list_alloc();
    view = variable_item_list_get_view(instance->variable_item_list);
    view_set_previous_callback(view, text_reader_previous_callback);
    view_dispatcher_add_view(instance->view_dispatcher, TextReaderViewConfigure, view);

    // Configurtion items
    VariableItem* item;
    instance->config_bias = false;
    instance->config_contrast = 32;
    instance->config_regulation_ratio = 0b101;
    // Bias
    item = variable_item_list_add(
        instance->variable_item_list,
        "Bias:",
        COUNT_OF(config_bias_value_text_reader),
        display_config_set_bias,
        instance);
    variable_item_set_current_value_index(item, 1);
    variable_item_set_current_value_text(item, config_bias_text_reader[1]);
    // Regulation Ratio
    item = variable_item_list_add(
        instance->variable_item_list,
        "Reg Ratio:",
        COUNT_OF(config_regulation_ratio_value_text_reader),
        display_config_set_regulation_ratio,
        instance);
    variable_item_set_current_value_index(item, 5);
    variable_item_set_current_value_text(item, config_regulation_ratio_text_reader[5]);
    // Contrast
    item = variable_item_list_add(
        instance->variable_item_list, "Contrast:", 64, display_config_set_contrast, instance);
    variable_item_set_current_value_index(item, 32);
    variable_item_set_current_value_text(item, "32");

    // Menu
    instance->submenu = submenu_alloc();
    view = submenu_get_view(instance->submenu);
    view_set_previous_callback(view, text_reader_exit_callback);
    view_dispatcher_add_view(instance->view_dispatcher, TextReaderViewSubmenu, view);
    submenu_add_item(
        instance->submenu,
        "Select File",
        TextReaderViewTextReader,
        text_reader_submenu_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Resume Last file",
        TextReaderViewConfigure,
        text_reader_submenu_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "This isn't a real module I'm just making stuff up right now",
        TextReaderViewConfigure,
        text_reader_submenu_callback,
        instance);

    return instance;
}

void text_reader_free(TextReader* instance) {
    view_dispatcher_remove_view(instance->view_dispatcher, TextReaderViewSubmenu);
    submenu_free(instance->submenu);

    view_dispatcher_remove_view(instance->view_dispatcher, TextReaderViewConfigure);
    variable_item_list_free(instance->variable_item_list);

    view_dispatcher_remove_view(instance->view_dispatcher, TextReaderViewTextReader);
    view_text_reader_free(instance->view_text_reader);

    view_dispatcher_free(instance->view_dispatcher);
    furi_record_close("gui");

    free(instance);
}

int32_t text_reader_run(TextReader* instance) {
    view_dispatcher_switch_to_view(instance->view_dispatcher, TextReaderViewSubmenu);
    view_dispatcher_run(instance->view_dispatcher);

    return 0;
}

int32_t text_reader_app(void* p) {
    TextReader* instance = text_reader_alloc();

    int32_t ret = text_reader_run(instance);

    text_reader_free(instance);

    return ret;
}
