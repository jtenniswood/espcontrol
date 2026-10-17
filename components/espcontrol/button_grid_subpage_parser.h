#pragma once

// Parser-only adapter for translation units without LVGL. Keep the production
// grid's parser as the single source of card normalization semantics.
#include "i18n_generated.h"
#define ESPCONTROL_CONFIG_PARSER_ONLY
#include "button_grid_config_parser.h"
#undef ESPCONTROL_CONFIG_PARSER_ONLY
#define ESPCONTROL_SUBPAGE_PARSER_ONLY
#include "button_grid_subpages.h"
#undef ESPCONTROL_SUBPAGE_PARSER_ONLY
