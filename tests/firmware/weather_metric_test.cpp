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

  for (bool forecast : {false, true}) {
    channels.clear();
    weather_metric_setup(slot, "temperature", {});
    assert(label.text == "Currently" && icon.text == "Thermometer");
    const std::string entity = forecast ? "weather.home" : "sensor.temperature";
    const std::string value_channel = entity + (forecast ? ":temperature" : "");
    const std::string unit_channel = entity + (forecast ? ":temperature_unit" : ":unit_of_measurement");
    subscribe_weather_metric(slot, entity, "temperature", forecast);
    assert(channels.size() == (forecast ? 3 : 2));
    emit(value_channel, "-5.250"); assert(reading.text == "-5.25");
    emit(unit_channel, "°C"); assert(reading.text == "-5.25 °C");
    emit(value_channel, "0"); assert(reading.text == "0 °C");
    emit(unit_channel, "°F"); assert(reading.text == "0 °F");
    emit(value_channel, "-0.0001"); assert(reading.text == "0 °F");
    for (const auto bad : {"", "unknown", "unavailable", "nan", "inf", "-inf", "2C", "1e20", "-1e20"}) {
      emit(value_channel, bad); assert(reading.text == "--");
    }
    emit(value_channel, "18.5"); assert(reading.text == "18.5 °F");
    if (forecast) {
      emit(entity, "unavailable"); assert(reading.text == "--");
      emit(value_channel, "-8.5"); assert(reading.text == "--");
      emit(entity, "sunny"); assert(reading.text == "-8.5 °F");
    }
    const auto before = reading.text;
    ++generation;
    emit(value_channel, "1"); assert(reading.text == before);
    --generation;
    reading.valid = false;
    emit(value_channel, "1"); assert(reading.text == before);
    reading.valid = true;
  }
}
