import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import switch
from esphome.const import CONF_ID, CONF_NAME

from . import (
    WR3223,                          # Hauptkomponente
    WR3223StatusComponent,           # Status-Polling-Komponente
    wr3223_ns,
    CONF_WR3223_ID,
    CONF_WR3223_STATUS_COMPONENT_ID,
    CONF_DEACTIVATE,
)

# C++ Klassen registrieren
WR3223StatusSwitch = wr3223_ns.class_("WR3223StatusSwitch", switch.Switch, cg.Component)
WR3223HeatPumpSwitch = wr3223_ns.class_("WR3223HeatPumpSwitch", WR3223StatusSwitch)
WR3223AdditionalHeatingSwitch = wr3223_ns.class_("WR3223AdditionalHeatingSwitch", WR3223StatusSwitch)
WR3223CoolingSwitch = wr3223_ns.class_("WR3223CoolingSwitch", WR3223StatusSwitch)

# Der neue Parameter-Switch
WR3223WPumpeFreiParamSwitch = wr3223_ns.class_("WR3223WPumpeFreiParamSwitch", switch.Switch, cg.Component)

CONF_SWITCHES = "switches"
CONF_HEAT_PUMP = "heat_pump"
CONF_ADDITIONAL_HEATING = "additional_heating"
CONF_COOLING = "cooling"
CONF_WPUMPE_FREI = "wpumpe_frei"


def _switch_schema(class_, default_name: str, default_icon: str):
    return (
        switch.switch_schema(class_, icon=default_icon)
        .extend({cv.Optional(CONF_NAME, default=default_name): cv.string_strict})
        .extend({cv.Optional(CONF_DEACTIVATE, default=False): cv.boolean})
        .extend(cv.COMPONENT_SCHEMA)
    )


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WR3223_ID): cv.use_id(WR3223),
        cv.GenerateID(CONF_WR3223_STATUS_COMPONENT_ID): cv.use_id(WR3223StatusComponent),
        cv.Optional(CONF_SWITCHES, default={}): cv.Schema(
            {
                cv.Optional(CONF_HEAT_PUMP, default={}): _switch_schema(
                    WR3223HeatPumpSwitch, "Wärmepumpe", "mdi:heat-pump-outline"
                ),
                cv.Optional(CONF_ADDITIONAL_HEATING, default={}): _switch_schema(
                    WR3223AdditionalHeatingSwitch, "Zusatzheizung", "mdi:heat-wave"
                ),
                cv.Optional(CONF_COOLING, default={}): _switch_schema(
                    WR3223CoolingSwitch, "Kühlung", "mdi:snowflake"
                ),
                cv.Optional(CONF_WPUMPE_FREI, default={}): _switch_schema(
                    WR3223WPumpeFreiParamSwitch, "Wärmepumpe Freigabe", "mdi:lock-open-outline"
                ),
            }
        ),
    }
)


# === FUNKTION 1: Für die klassischen Status-basierten Switches ===
async def build_status_switch(switches_conf, key, class_, status_comp):
    conf = switches_conf.get(key, {})
    if not conf or conf.get(CONF_DEACTIVATE):
        return
    var = await switch.new_switch(conf)
    await cg.register_component(var, conf)
    cg.add(var.set_status_component(status_comp))


# === FUNKTION 2: Für den neuen Parameter-basierten EEPROM-Switch ===
async def build_param_switch(switches_conf, key, class_, wr3223_parent, status_comp):
    conf = switches_conf.get(key, {})
    if not conf or conf.get(CONF_DEACTIVATE):
        return
    var = await switch.new_switch(conf)
    await cg.register_component(var, conf)
    
    # Verknüpfung mit der Hauptkomponente für die Sende-Befehle
    cg.add(var.set_wr3223_parent(wr3223_parent))
    
    # Verknüpfung mit der Statuskomponente für die ausgelesenen Updates
    cg.add(var.set_status_component(status_comp))


async def to_code(config):
    # IDs aus der Konfiguration auflösen
    wr3223_parent = await cg.get_variable(config[CONF_WR3223_ID])
    status_comp = await cg.get_variable(config[CONF_WR3223_STATUS_COMPONENT_ID])
    switches_conf = config.get(CONF_SWITCHES, {})

    # 1. Die klassischen Status-Switches bauen (wie gehabt)
    await build_status_switch(switches_conf, CONF_HEAT_PUMP, WR3223HeatPumpSwitch, status_comp)
    await build_status_switch(switches_conf, CONF_ADDITIONAL_HEATING, WR3223AdditionalHeatingSwitch, status_comp)
    await build_status_switch(switches_conf, CONF_COOLING, WR3223CoolingSwitch, status_comp)

    # 2. Den neuen Parameter-Switch über die separate Logik bauen
    await build_param_switch(switches_conf, CONF_WPUMPE_FREI, WR3223WPumpeFreiParamSwitch, wr3223_parent, status_comp)
