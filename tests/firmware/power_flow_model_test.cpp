#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>
#include <type_traits>

#include "power_flow_model.h"

using namespace espcontrol::power;
using V = CalculationValidity;
constexpr double MISSING = std::numeric_limits<double>::quiet_NaN();
static_assert(std::is_trivially_copyable<Model>::value, "model stores no dynamic ownership");
static_assert(sizeof(Model) <= 256, "keep per-card power state bounded");

struct Scenario {
  const char *name;
  std::array<double, 4> watts;
  Polarity polarity;
  std::array<double, 7> inferred;
  V validity;
  const char *secondary;
  ReadingValidity absent_validity = ReadingValidity::MISSING;
};

// Solar, Home, Grid, Battery; then SH, SB, SG, BH, BG, GH, GB.
const Scenario scenarios[] = {
  {"solar home", {1000,1000,0,0}, {}, {1000,0,0,0,0,0,0}, V::VALID, "Idle"},
  {"solar battery", {1500,1000,0,-500}, {}, {1000,500,0,0,0,0,0}, V::VALID, "Charging"},
  {"solar grid", {1500,1000,-500,0}, {}, {1000,0,500,0,0,0,0}, V::VALID, "Exporting"},
  {"battery home", {0,1000,0,1000}, {}, {0,0,0,1000,0,0,0}, V::VALID, "Discharging"},
  {"battery grid", {0,0,-500,500}, {}, {0,0,0,0,500,0,0}, V::VALID, "Discharging · Exporting"},
  {"grid home", {0,1000,1000,0}, {}, {0,0,0,0,0,1000,0}, V::VALID, "Importing"},
  {"grid battery", {0,0,500,-500}, {}, {0,0,0,0,0,0,500}, V::VALID, "Charging · Importing"},
  {"import charging", {300,1000,1000,-300}, {}, {300,0,0,0,0,700,300}, V::VALID, "Charging · Importing"},
  {"discharge export", {300,700,-400,800}, {}, {300,0,0,400,400,0,0}, V::VALID, "Discharging · Exporting"},
  {"solar charge export", {1800,1000,-300,-500}, {}, {1000,500,300,0,0,0,0}, V::VALID, "Charging · Exporting"},
  {"zero", {0,0,0,0}, {}, {}, V::VALID, "Idle"},
  {"near zero", {.5,.5,.5,-.5}, {}, {}, V::VALID, "Idle"},
  {"idle boundary", {0,1,1,0}, {}, {}, V::VALID, "Idle"},
  {"above idle", {0,1.1,1.1,0}, {}, {0,0,0,0,0,1.1,0}, V::VALID, "Importing"},
  {"accepted residual", {300,295,0,0}, {}, {295,0,0,0,0,0,0}, V::VALID, "Idle"},
  {"absolute boundary", {100,90,0,0}, {}, {90,0,0,0,0,0,0}, V::VALID, "Idle"},
  {"absolute outside", {100,89,0,0}, {}, {}, V::INCONSISTENT, "Idle"},
  {"negative absolute boundary", {90,100,0,0}, {}, {90,0,0,0,0,0,0}, V::VALID, "Idle"},
  {"negative absolute outside", {89,100,0,0}, {}, {}, V::INCONSISTENT, "Idle"},
  {"relative boundary", {1200,1176,0,0}, {}, {1176,0,0,0,0,0,0}, V::VALID, "Idle"},
  {"relative outside", {1200,1175,0,0}, {}, {}, V::INCONSISTENT, "Idle"},
  {"negative relative boundary", {1176,1200,0,0}, {}, {1176,0,0,0,0,0,0}, V::VALID, "Idle"},
  {"negative relative outside", {1175,1200,0,0}, {}, {}, V::INCONSISTENT, "Idle"},
  {"large imbalance retains status", {500,1000,200,-100}, {}, {}, V::INCONSISTENT, "Charging · Importing"},
  {"missing solar", {MISSING,1000,700,300}, {}, {}, V::UNAVAILABLE, "Discharging · Importing"},
  {"missing home", {500,MISSING,-200,-300}, {}, {}, V::UNAVAILABLE, "Charging · Exporting"},
  {"missing grid", {500,500,MISSING,0}, {}, {}, V::UNAVAILABLE, "Unavailable"},
  {"missing battery", {500,1000,500,MISSING}, {}, {}, V::UNAVAILABLE, "Importing"},
  {"invalid grid", {0,1000,MISSING,1000}, {}, {}, V::UNAVAILABLE, "Discharging", ReadingValidity::INVALID},
  {"both statuses missing", {500,500,MISSING,MISSING}, {}, {}, V::UNAVAILABLE, "Unavailable"},
  {"grid inversion", {0,1000,-1000,0}, {true,false}, {0,0,0,0,0,1000,0}, V::VALID, "Importing"},
  {"battery inversion", {0,1000,0,-1000}, {false,true}, {0,0,0,1000,0,0,0}, V::VALID, "Discharging"},
  {"both inversions", {300,1000,-1000,300}, {true,true}, {300,0,0,0,0,700,300}, V::VALID, "Charging · Importing"},
  {"no automatic inversion", {300,1000,-1000,300}, {}, {}, V::INCONSISTENT, "Discharging · Exporting"},
  {"hidden allocation consumes capacity", {1000,999.5,50,-50}, {}, {999.5,.5,0,0,0,0,49.5}, V::VALID, "Charging · Importing"},
};

void near(double actual, double expected) { assert(std::abs(actual - expected) < 1e-8); }

void assert_capacity(const Calculation &result) {
  const auto &f = result.inferred_w;
  const auto &n = result.nodes;
  auto capacity = [&](size_t i, bool sink) {
    return n[i].valid() ? std::max(0.0, n[i].value * (sink ? -1 : 1)) : 0.0;
  };
  assert(f[0] + f[1] + f[2] <= capacity(0,false) + 1e-8);
  assert(f[3] + f[4] <= capacity(3,false) + 1e-8);
  assert(f[5] + f[6] <= capacity(2,false) + 1e-8);
  assert(f[0] + f[3] + f[5] <= capacity(1,false) + 1e-8);
  assert(f[1] + f[6] <= capacity(3,true) + 1e-8);
  assert(f[2] + f[4] <= capacity(2,true) + 1e-8);
  assert(!(result.edge_active(Edge::GRID_BATTERY) && result.edge_active(Edge::BATTERY_GRID)));
}

void table_tests() {
  for (const auto &test : scenarios) for (const char *unit : {"W", "kW"}) {
    std::array<Reading,4> readings{};
    for (size_t i = 0; i < readings.size(); ++i) {
      if (std::isnan(test.watts[i])) { readings[i].validity = test.absent_validity; continue; }
      std::ostringstream state;
      state.precision(17);
      state << test.watts[i] / (std::string(unit) == "kW" ? 1000 : 1);
      readings[i] = parse_power(state.str(), unit, static_cast<Node>(i));
    }
    const auto result = calculate(readings, test.polarity);
    if (result.validity != test.validity) std::cerr << test.name << " " << unit << '\n';
    assert(result.validity == test.validity);
    assert(result.secondary_label() == test.secondary);
    for (size_t i = 0; i < result.inferred_w.size(); ++i) {
      assert(std::isfinite(result.inferred_w[i]) && result.inferred_w[i] >= 0);
      near(result.inferred_w[i], test.inferred[i]);
      assert(result.edge_active(static_cast<Edge>(i)) == (test.inferred[i] > IDLE_THRESHOLD_W));
    }
    for (size_t i = 0; i < readings.size(); ++i) {
      assert(result.nodes[i].valid() == readings[i].valid());
      if (readings[i].valid()) near(std::abs(result.nodes[i].value), std::abs(test.watts[i]));
    }
    assert_capacity(result);
  }
  // Sweep balanced source/sink combinations to test conservation beyond examples.
  for (double solar : {0.,.5,1.,1.1,100.,1000.})
    for (double grid : {-500.,-1.,0.,1.,500.})
      for (double battery : {-500.,-1.,0.,1.,500.}) {
        const double home = solar + grid + battery;
        if (home < 0) continue;
        auto result = calculate({{{solar,ReadingValidity::VALID},{home,ReadingValidity::VALID},
                                  {grid,ReadingValidity::VALID},{battery,ReadingValidity::VALID}}});
        assert(result.validity == V::VALID);
        near(result.residual_w, 0);
        assert_capacity(result);
        if ((solar == 0 || solar > 1) && (grid == 0 || std::abs(grid) > 1) &&
            (battery == 0 || std::abs(battery) > 1) && (home == 0 || home > 1)) {
          double total = 0;
          for (double value : result.inferred_w) total += value;
          near(total, solar + std::max(grid,0.0) + std::max(battery,0.0));
        }
      }
}

void parsing_tests() {
  for (const char *state : {"", " \t "})
    assert(parse_power(state,"W",Node::HOME).validity == ReadingValidity::MISSING);
  for (const char *state : {"NaN", "nan", "inf", "-inf", "1e309", "1e-400", "100 W", "12bad",
                           "1 2", "1,2", "0x10", "0x1p2", "+", ".", "1e", "1e+", "--1"})
    assert(parse_power(state,"W",Node::GRID).validity == ReadingValidity::INVALID);
  for (const char *unit : {"", "KW", "w", "MW", "Wh", "kWh", "%", "watts"})
    assert(!parse_power("1",unit,Node::HOME).valid());
  near(parse_power(" \t+1.25e1\r\n", " kW ", Node::HOME).value, 12500);
  near(parse_power("-.5", "W", Node::GRID).value, -.5);
  near(parse_power("-1", "W", Node::SOLAR).value, -1); // tolerated idle-band noise
  assert(!parse_power("-1.01", "W", Node::SOLAR).valid());
  assert(!parse_power("-2", "W", Node::HOME).valid());
  assert(!parse_power("1e300", "kW", Node::GRID).valid());
  assert(!parse_power("2147483", "W", Node::GRID).valid());
  assert(normalize_power({MAX_POWER_W,ReadingValidity::VALID},"W",Node::HOME).valid());
  assert(!normalize_power({MAX_POWER_W+1,ReadingValidity::VALID},"W",Node::HOME).valid());
  for (double value : {std::numeric_limits<double>::infinity(), MISSING})
    assert(!normalize_power({value,ReadingValidity::VALID},"W",Node::HOME).valid());
  for (const char *state : {"0", "78", "100"}) assert(parse_soc(state,"%").valid());
  for (const char *state : {"-1", "100.01", "101", "NaN", "inf", "78%"}) assert(!parse_soc(state,"%").valid());
  assert(!parse_soc("78","W").valid());
  assert(parse_soc("unavailable","%").validity == ReadingValidity::UNAVAILABLE);
  assert(parse_power("unknown","W",Node::GRID).validity == ReadingValidity::UNKNOWN);
  assert(parse_power("unavailable","W",Node::GRID).validity == ReadingValidity::UNAVAILABLE);
  assert(parse_power("stale","W",Node::GRID).validity == ReadingValidity::STALE);
}

void status_tests() {
  const char *expected[4][4] = {
    {"Unavailable","Unavailable","Importing","Exporting"},
    {"Unavailable","Idle","Importing","Exporting"},
    {"Charging","Charging","Charging · Importing","Charging · Exporting"},
    {"Discharging","Discharging","Discharging · Importing","Discharging · Exporting"},
  };
  const double battery[] = {MISSING,0,-10,10}, grid[] = {MISSING,0,10,-10};
  for (size_t b = 0; b < 4; ++b) for (size_t g = 0; g < 4; ++g) {
    std::array<Reading,4> nodes{};
    if (b) nodes[3] = {battery[b],ReadingValidity::VALID};
    if (g) nodes[2] = {grid[g],ReadingValidity::VALID};
    const auto result = calculate(nodes);
    assert(result.validity == V::UNAVAILABLE);
    assert(result.secondary_label() == expected[b][g]);
  }
  Calculation translated;
  translated.battery_status = Status::CHARGING;
  translated.grid_status = Status::EXPORTING;
  set_espcontrol_language("it");
  assert(translated.secondary_label() == "In carica · Esportazione");
  set_espcontrol_language("en");
}

void transition_tests() {
  Model model;
  model.set_power(Node::SOLAR,"0","W");
  model.set_power(Node::HOME,"1000","W");
  model.set_power(Node::GRID,"1000","W");
  model.set_power(Node::BATTERY,"0","W");
  assert(model.calculation.edge_active(Edge::GRID_HOME));
  model.set_power(Node::SOLAR,"bad","W");
  assert(model.calculation.validity == V::UNAVAILABLE);
  assert(model.calculation.nodes[0].validity == ReadingValidity::INVALID);
  assert(model.calculation.nodes[1].value == 1000);
  assert(model.calculation.secondary_label() == "Importing");
  for (double value : model.calculation.inferred_w) assert(value == 0);
  model.set_power(Node::SOLAR,"0","W");
  assert(model.calculation.edge_active(Edge::GRID_HOME));
  model.set_power(Node::HOME,"2000","W");
  assert(model.calculation.validity == V::INCONSISTENT);
  for (double value : model.calculation.inferred_w) assert(value == 0);
  model.set_power(Node::HOME,"1000","W");
  assert(model.calculation.edge_active(Edge::GRID_HOME));
  model.set_soc("78","%");
  assert(model.battery_soc.value == 78);
  model.set_soc("101","%");
  assert(!model.battery_soc.valid());
  assert(model.calculation.edge_active(Edge::GRID_HOME));
  model.set_polarity({true,false});
  assert(model.calculation.validity == V::INCONSISTENT);
  model.set_power(Node::GRID,"-1000","W");
  assert(model.calculation.edge_active(Edge::GRID_HOME));
  // Recalculation never inverts the stored raw normalized value a second time.
  model.set_polarity({true,false});
  assert(model.calculation.edge_active(Edge::GRID_HOME));
  model.invalidate();
  assert(model.calculation.validity == V::UNAVAILABLE);
  assert(model.calculation.secondary_label() == "Unavailable");
  for (const auto &reading : model.readings) assert(!reading.valid());
  assert(!model.battery_soc.valid());
  assert(model.polarity.invert_grid); // reconnect preserves configuration
}

void partial_system_tests() {
  const std::array<std::array<size_t,2>,7> endpoints={{{0,1},{0,3},{0,2},{3,1},{3,2},{2,1},{2,3}}};
  // Exhaust every installed/configured mask: no silent zero for unmeasured
  // installed equipment, even when its old raw Reading still contains a value.
  for(uint8_t installed=0;installed<16;++installed)for(uint8_t configured=0;configured<16;++configured){
    std::array<Reading,4> input{};
    for(auto &n:input)n={0,ReadingValidity::VALID};
    const auto c=calculate(input,{}, {installed,configured});
    const uint8_t normalized=installed|component_bit(Node::HOME);
    assert(c.capabilities.installed==normalized&&c.capabilities.includes(Node::HOME));
    assert(c.balance_checkable==((normalized&configured)==normalized));
    assert(c.validity==((normalized&configured)==normalized?V::VALID:V::READINGS_ONLY));
    for(size_t n=0;n<4;++n){
      const auto expected=!(normalized&(1u<<n))?ReadingValidity::EXCLUDED:
        !(configured&(1u<<n))?ReadingValidity::UNMEASURED:ReadingValidity::VALID;
      assert(c.nodes[n].validity==expected);
    }
    for(double flow:c.inferred_w)assert(flow==0);
  }
  // Every physical topology, both polarities, balanced/unbalanced fractional
  // snapshots; check signs, capacities, exclusion and exact conservation.
  for(uint8_t mask=2;mask<16;++mask)if(mask&2)for(double solar:{0.,.3,1.,100.,1000.})
    for(double home:{0.,.3,1.,100.,1000.})for(double grid:{-500.,-.3,0.,.3,500.})
      for(double battery:{-500.,-.3,0.,.3,500.})for(bool inverted:{false,true}){
        std::array<Reading,4> input={{{solar,ReadingValidity::VALID},{home,ReadingValidity::VALID},
          {inverted?-grid:grid,ReadingValidity::VALID},{inverted?-battery:battery,ReadingValidity::VALID}}};
        for(size_t n=0;n<4;++n)if(!(mask&(1u<<n)))input[n]={MISSING,ReadingValidity::INVALID};
        const auto c=calculate(input,{inverted,inverted},{mask,mask});assert(c.balance_checkable);
        const double s=mask&1?solar:0,h=mask&2?home:0,g=mask&4?grid:0,b=mask&8?battery:0;
        near(c.residual_w,s+g+b-h);
        const double sources=s+std::max(g,0.)+std::max(b,0.);
        const double sinks=h+std::max(-g,0.)+std::max(-b,0.);
        near(c.tolerance_w,std::max(10.,.02*std::max(sources,sinks)));
        assert(c.validity==(std::abs(c.residual_w)<=c.tolerance_w?V::VALID:V::INCONSISTENT));
        assert_capacity(c);
        double total=0;
        for(size_t edge=0;edge<7;++edge){
          total+=c.inferred_w[edge];
          if(c.inferred_w[edge]>0)for(auto n:endpoints[edge])assert(mask&(1u<<n));
          if(c.validity!=V::VALID)assert(c.inferred_w[edge]==0);
        }
        if(c.validity==V::VALID&&c.residual_w==0&&
          (s==0||s>1)&&(h==0||h>1)&&(g==0||std::abs(g)>1)&&(b==0||std::abs(b)>1))near(total,sources);
      }
  // All seven directions with their endpoints and mandatory measured Home.
  // Non-Home routes require an actual zero Home reading, never a synthetic one.
  for(size_t edge=0;edge<7;++edge){
    std::array<Reading,4> n{};n[1]={0,ReadingValidity::VALID};const auto source=endpoints[edge][0],sink=endpoints[edge][1];
    n[source]={500,ReadingValidity::VALID};n[sink]={sink==1?500.:-500.,ReadingValidity::VALID};
    const uint8_t mask=(1u<<source)|(1u<<sink)|component_bit(Node::HOME);
    const auto c=calculate(n,{}, {mask,mask});assert(c.validity==V::VALID&&c.inference==InferenceKind::UNIQUE);
    for(size_t e=0;e<7;++e)near(c.inferred_w[e],edge==e?500.:0.);
  }
  std::array<Reading,4> full={{{300,ReadingValidity::VALID},{1000,ReadingValidity::VALID},{1000,ReadingValidity::VALID},{-300,ReadingValidity::VALID}}};
  assert(calculate(full).inference==InferenceKind::SOLAR_FIRST);
  for(auto validity:{ReadingValidity::MISSING,ReadingValidity::INVALID,ReadingValidity::UNKNOWN,ReadingValidity::UNAVAILABLE,ReadingValidity::STALE}){
    auto partial=full;partial[0].validity=validity;const auto c=calculate(partial);
    assert(!c.balance_checkable&&c.validity==V::UNAVAILABLE&&c.nodes[0].validity==validity);
    assert(c.secondary_label()=="Charging · Importing");for(double f:c.inferred_w)assert(f==0);
  }
  auto isolated=calculate({{{8,ReadingValidity::VALID},{},{},{}}},{},{1,1});
  assert(isolated.validity==V::READINGS_ONLY&&!isolated.idle()&&isolated.inference==InferenceKind::NONE);
  assert(isolated.nodes[1].validity==ReadingValidity::UNMEASURED);
  assert(!calculate({{{0,ReadingValidity::VALID},{},{},{}}},{},{1,1}).idle());
  // Every optional-component topology keeps Home visible. Its sensor can be
  // absent or unavailable; even an old excluded-Home mask cannot synthesize zero.
  for(uint8_t mask=0;mask<16;++mask)if(!(mask&2)){
    std::array<Reading,4> zeros{};for(auto &n:zeros)n={0,ReadingValidity::VALID};
    const auto unmeasured=calculate(zeros,{}, {mask,mask});
    assert(unmeasured.capabilities.includes(Node::HOME));
    assert(unmeasured.nodes[1].validity==ReadingValidity::UNMEASURED);
    assert(unmeasured.validity==V::READINGS_ONLY&&!unmeasured.balance_checkable);
    for(double f:unmeasured.inferred_w)assert(f==0);
    for(auto state:{ReadingValidity::MISSING,ReadingValidity::UNAVAILABLE,ReadingValidity::UNKNOWN,ReadingValidity::INVALID,ReadingValidity::STALE}){
      zeros[1]={0,state};const auto unavailable=calculate(zeros,{}, {mask,uint8_t(mask|2)});
      assert(unavailable.nodes[1].validity==state&&unavailable.validity==V::UNAVAILABLE);
      for(double f:unavailable.inferred_w)assert(f==0);
    }
    zeros[1]={0,ReadingValidity::VALID};const auto measured=calculate(zeros,{}, {mask,uint8_t(mask|2)});
    assert(measured.nodes[1].valid()&&measured.nodes[1].value==0&&measured.idle());
    Model legacy;legacy.set_capabilities({mask,mask});
    assert(legacy.capabilities.installed==uint8_t(mask|2));
    assert(legacy.calculation.nodes[1].validity==ReadingValidity::UNMEASURED);
  }
  auto c=calculate(full,{}, {15,14});assert(c.validity==V::READINGS_ONLY);
  assert(c.nodes[0].validity==ReadingValidity::UNMEASURED&&c.secondary_label()=="Charging · Importing");
  Model m;m.set_power(Node::SOLAR,"0","W");m.set_power(Node::HOME,"1000","W");m.set_power(Node::GRID,"1000","W");m.set_power(Node::BATTERY,"0","W");
  m.set_capabilities({6,6});assert(m.calculation.edge_active(Edge::GRID_HOME));
  m.set_capabilities({7,7});assert(m.calculation.validity==V::UNAVAILABLE); // reenabling cannot revive old Solar
  m.set_power(Node::SOLAR,"0","W");assert(m.calculation.validity==V::VALID);
  m.set_capabilities({7,3});assert(m.calculation.validity==V::READINGS_ONLY);
  m.invalidate();assert(m.capabilities.installed==7&&m.capabilities.configured==3);
}
void integer_tests() {
  for(auto test:{std::pair<double,const char*>{1234.6,"1235"},{-328.7,"-329"},{.3,"0"},{-.3,"0"},
    {.5,"1"},{-.5,"-1"},{1.5,"2"},{-1.5,"-2"},{-.0,"0"}})
    assert(integer_watts({test.first,ReadingValidity::VALID})==test.second);
  assert(integer_watts({MISSING,ReadingValidity::VALID})=="--");
  const auto c=calculate({{{1234.6,ReadingValidity::VALID},{1234.2,ReadingValidity::VALID},{0,ReadingValidity::VALID},{0,ReadingValidity::VALID}}});
  near(c.residual_w,.4);near(c.inferred_w[0],1234.2);assert(integer_watts(c.nodes[0])=="1235");
  near(c.nodes[0].value,1234.6); // formatting never feeds rounded watts back into the model
}

int main() {
  partial_system_tests();
  integer_tests();
  table_tests();
  parsing_tests();
  status_tests();
  transition_tests();
  std::cout << "35 scenarios in W/kW; conservation, parsing, statuses and lifecycle passed\n";
}
