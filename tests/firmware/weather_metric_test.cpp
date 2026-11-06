#include "weather_reading_test_support.h"
#include "button_grid_weather_metric.h"

int main() {
  lv_obj_t button, icon, label, container, reading, unit;
  button.text_color = 0x212121;
  BtnSlot slot{&button, &icon, &label, &container, &reading, &unit};
  for (const std::string mode : {"pressure", "humidity", "visibility"}) {
    weather_metric_setup(slot, mode, {});
    assert(label.text == (mode == "pressure" ? "Air Pressure" : mode == "humidity" ? "Humidity" : "Visibility"));
    assert(icon.text == (mode == "pressure" ? "Gauge" : mode == "humidity" ? "Water Percent" : "Weather Hazy"));
    assert(!(icon.flags & LV_OBJ_FLAG_HIDDEN) && !(container.flags & LV_OBJ_FLAG_HIDDEN));
    assert(unit.flags & LV_OBJ_FLAG_HIDDEN);
    assert(reading.text == "--" && reading.text_opa == LV_OPA_70);
    assert(reading.text_color == button.text_color && reading.font == &font);
    assert(label.y == -(font.line_height + 2));
    channels.clear();
    const std::string entity = "weather.home";
    subscribe_weather_metric(slot, entity, mode, true);
    emit(entity, "cloudy");
    emit(entity + ":" + mode, "0");
    assert(reading.text == (mode == "humidity" ? "0 %" : "0"));
    if (mode != "humidity") emit(entity + ":" + mode + "_unit", mode == "pressure" ? "hPa" : "km");
    const std::string value = mode == "pressure" ? "1013.250" : mode == "humidity" ? "65.50" : "10.250";
    const std::string expected = mode == "pressure" ? "1013.25 hPa" : mode == "humidity" ? "65.5 %" : "10.25 km";
    emit(entity + ":" + mode, value); assert(reading.text == expected);
    emit(entity, "unavailable"); assert(reading.text == "--");
    emit(entity + ":" + mode, value); assert(reading.text == "--");
    emit(entity, "sunny"); assert(reading.text == expected);
    for (const auto bad : {"", "unknown", "unavailable", "nan", "inf", "-1", "2km"}) {
      emit(entity + ":" + mode, bad); assert(reading.text == "--");
    }
    if (mode == "humidity") { emit(entity + ":humidity", "101"); assert(reading.text == "--"); }
    emit(entity + ":" + mode, value); assert(reading.text == expected);
    ++generation;
    emit(entity + ":" + mode, "0"); assert(reading.text == expected);
    --generation;
    reading.valid = false;
    emit(entity + ":" + mode, "0"); assert(reading.text == expected);
    reading.valid = true;
  }
  for (const std::string mode : {"pressure", "humidity"}) {
    channels.clear();
    subscribe_weather_metric(slot, "sensor.reading", mode, false);
    assert(channels.size() == 2);
    emit("sensor.reading:unit_of_measurement", mode == "pressure" ? "inHg" : "%");
    emit("sensor.reading", "29.920");
    assert(reading.text == (mode == "pressure" ? "29.92 inHg" : "29.92 %"));
    emit("sensor.reading", "unavailable"); assert(reading.text == "--");
    emit("sensor.reading", "0");
    assert(reading.text == (mode == "pressure" ? "0 inHg" : "0 %"));
    emit("sensor.reading:unit_of_measurement", "unknown"); assert(reading.text == "0");
  }
}
