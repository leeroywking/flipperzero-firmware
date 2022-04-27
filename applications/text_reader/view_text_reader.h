#pragma once

#include <stdint.h>
#include <gui/view.h>

typedef struct ViewTextReader ViewTextReader;

ViewTextReader* view_text_reader_alloc();

void view_text_reader_free(ViewTextReader* instance);

View* view_text_reader_get_view(ViewTextReader* instance);
