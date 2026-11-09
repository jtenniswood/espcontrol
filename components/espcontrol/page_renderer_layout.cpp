#include "page_renderer_layout.h"

#include <algorithm>
#include <cstdlib>

#include "button_grid_subpage_parser.h"

namespace espcontrol::pages {
namespace {
void place(PageCellLayout &cell,CardId card,size_t position,PageViewGeometry geometry,
           int rows,int columns,bool back=false) {
  cell={card,static_cast<uint8_t>(position%geometry.columns),static_cast<uint8_t>(position/geometry.columns),
        static_cast<uint8_t>(columns),static_cast<uint8_t>(rows),true,back};
}
}  // namespace
bool compile_page_layout(const PageStore &store,PageId page_id,PageViewGeometry geometry,
                         PageCellLayout *cells,size_t capacity,PageBackAppearance *back) {
  const auto *page=store.page(page_id);
  if (cells==nullptr || back==nullptr || page==nullptr || !page->navigable ||
      geometry.columns==0 || geometry.columns>UINT8_MAX || geometry.visible_slots==0 ||
      geometry.visible_slots>capacity || geometry.visible_slots>MAX_GRID_SLOTS ||
      geometry.visible_slots>UINT8_MAX) return false;
  std::fill_n(cells,capacity,PageCellLayout{});
  *back={};
  const std::string order(store.string(page->order));
  if (page_id==0) {
    OrderResult parsed,normalized;
    parse_order_string(order,geometry.visible_slots,parsed);
    clear_spanned_cells(parsed,geometry.visible_slots,geometry.columns,normalized);
    for (size_t position=0;position<geometry.visible_slots;++position) {
      const int number=normalized.positions[position];
      if (number<1 || static_cast<size_t>(number)>page->card_count) continue;
      const auto id=static_cast<CardId>(page->first_card+number-1);
      // Existing root widgets are keyed by card, so repeated references finish
      // at their last surviving position rather than creating extra widgets.
      for (size_t earlier=0;earlier<position;++earlier)
        if(cells[earlier].card==id) cells[earlier]={};
      place(cells[position],id,position,geometry,normalized.row_span[number-1],normalized.col_span[number-1]);
    }
    return true;
  }
  SubpageOrder parsed;
  parse_subpage_order(order,geometry.visible_slots,page->card_count,parsed);
  normalize_subpage_order_spans(parsed,geometry.visible_slots,geometry.columns);
  place(cells[parsed.back_pos],NO_CARD,parsed.back_pos,geometry,parsed.back_row_span,parsed.back_col_span,true);
  back->custom_label=get_subpage_back_custom_label(order);
  const std::string color=get_subpage_back_color(order);
  back->custom_color=!color.empty();
  if(back->custom_color)back->background_color=static_cast<uint32_t>(std::strtoul(color.c_str(),nullptr,16));
  for(size_t position=0;position<geometry.visible_slots;++position) {
    const int number=parsed.positions[position];
    if(number<1 || static_cast<size_t>(number)>page->card_count)continue;
    const size_t cell=parsed.has_back_token?position:position+1;
    if(cell>=geometry.visible_slots)continue;
    place(cells[cell],static_cast<CardId>(page->first_card+number-1),cell,geometry,
          parsed.row_span[number-1],parsed.col_span[number-1]);
  }
  return true;
}
}  // namespace espcontrol::pages
