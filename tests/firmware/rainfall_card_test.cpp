#include "weather_reading_test_support.h"
#include "button_grid_rainfall.h"
int main() {
  using espcontrol::rainfall::amount;
  using espcontrol::rainfall::value;
  assert(amount(0,"mm")=="0 mm");
  assert(amount(2.4,"mm")=="2.4 mm");
  assert(amount(0.001,"in")=="0.001 in");
  assert(amount(2.3456,"mm")=="2.346 mm");
  for (auto text:{"", "unknown", "unavailable", "nan", "inf", "2mm", "-1"})
    assert(amount(value(text),"mm")=="--");
  assert(amount(3,"unknown")=="3");
  lv_obj_t button,icon,label,container,reading,unit;
  button.padding=0; button.text_color=0x212121;
  BtnSlot slot{&button,&icon,&label,&container,&reading,&unit};
  rainfall_setup(slot, {});
  assert(button.padding==16 && label.text=="Rainfall" && icon.text=="Weather Rainy");
  assert(reading.font==&font && reading.text_opa==LV_OPA_70 && reading.text_color==button.text_color);
  assert(unit.flags & LV_OBJ_FLAG_HIDDEN);
  assert(label.y==-(font.line_height+2));
  subscribe_rainfall_sensor(slot,"sensor.rainfall_today");
  emit("sensor.rainfall_today:unit_of_measurement","mm");
  assert(reading.text=="--");
  emit("sensor.rainfall_today","0"); assert(reading.text=="0 mm");
  emit("sensor.rainfall_today","2.40"); assert(reading.text=="2.4 mm");
  emit("sensor.rainfall_today:unit_of_measurement","in"); assert(reading.text=="2.4 in");
  emit("sensor.rainfall_today","unavailable"); assert(reading.text=="--");
  emit("sensor.rainfall_today","0.025"); assert(reading.text=="0.025 in");
  ++generation;
  emit("sensor.rainfall_today","10"); assert(reading.text=="0.025 in");
  --generation; reading.valid=false;
  emit("sensor.rainfall_today","20"); assert(reading.text=="0.025 in");

  reading.valid=true;
  subscriptions.clear(); channels.clear();
  subscribe_rainfall_sensor(slot,"sensor.rainfall_today","in");
  assert(!subscriptions.count("unit_of_measurement"));
  emit("sensor.rainfall_today","2.40"); assert(reading.text=="2.4 in");
  emit("sensor.rainfall_today","0"); assert(reading.text=="0 in");
  emit("sensor.rainfall_today","unavailable"); assert(reading.text=="--");

  subscriptions.clear(); channels.clear();
  subscribe_rainfall_sensor(slot,"input_number.rainfall","mm");
  emit("input_number.rainfall","3.5"); assert(reading.text=="3.5 mm");

  subscriptions.clear(); channels.clear();
  subscribe_rainfall_sensor(slot,"sensor.rainfall_today","");
  emit("sensor.rainfall_today","1.2"); assert(reading.text=="1.2");
  emit("sensor.rainfall_today:unit_of_measurement","mm"); assert(reading.text=="1.2 mm");
}
