import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import number
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_MIN_VALUE,
    CONF_MAX_VALUE,
    CONF_STEP,
    CONF_ENTITY_CATEGORY,
    CONF_UNIT_OF_MEASUREMENT,
    ENTITY_CATEGORY_CONFIG,
    UNIT_CELSIUS,
    UNIT_PERCENT,
)

from . import WR3223, wr3223_ns, CONF_WR3223_ID, CONF_DEACTIVATE

WR3223VentSpeedNumber = wr3223_ns.class_(
    "WR3223VentSpeedNumber", number.Number, cg.Component
)
WR3223ParameterNumber = wr3223_ns.class_(
    "WR3223ParameterNumber", number.Number, cg.Component
)

CONF_NUMBERS = "numbers"
CONF_PARAMETERS = "parameters"

# Neue Optionen für bedingte Erstellung
CONF_EWT_PRESENT = "ewt_present"
CONF_AIR_BALANCE_PRESENT = "air_balance_present"

# Lüfterstufen
CONF_VENT_LEVEL_1_SPEED = "vent_level_1_speed"
CONF_VENT_LEVEL_2_SPEED = "vent_level_2_speed"
CONF_VENT_LEVEL_3_SPEED = "vent_level_3_speed"

# Parameter - Luftbalance
CONF_SUPPLY_AIR_DIFF = "supply_air_diff"      # LD - Zuluft +/-
CONF_EXHAUST_AIR_DIFF = "exhaust_air_diff"    # Ld - Abluft +/-

# Parameter - EWT
CONF_EWT_SUMMER = "ewt_summer"                # ES - EWT Sommer
CONF_EWT_WINTER = "ewt_winter"                # EW - EWT Winter
CONF_BRINE_PUMP_ON = "brine_pump_on"          # EE - Solepumpe Ein
CONF_BRINE_PUMP_OFF = "brine_pump_off"        # EA - Solepumpe Aus
CONF_SUMMER_STOP = "summer_stop"              # Es - Sommer Stopp


def _speed_schema(default_name: str):
    return (
        number.number_schema(
            WR3223VentSpeedNumber,
            unit_of_measurement="%",
            icon="mdi:fan",
        )
        .extend(
            {
                cv.Optional(CONF_NAME, default=default_name): cv.string_strict,
                cv.Optional(CONF_DEACTIVATE, default=False): cv.boolean,
                cv.Optional(CONF_MIN_VALUE, default=40): cv.int_,
                cv.Optional(CONF_MAX_VALUE, default=100): cv.int_,
                cv.Optional(CONF_STEP, default=1): cv.int_,
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
            }
        )
        .extend(cv.COMPONENT_SCHEMA)
    )


def _parameter_schema(default_name: str, command: str, min_val: float, max_val: float, 
                      step: float, unit: str, icon: str):
    return (
        number.number_schema(
            WR3223ParameterNumber,
            unit_of_measurement=unit,
            icon=icon,
        )
        .extend(
            {
                cv.Optional(CONF_NAME, default=default_name): cv.string_strict,
                cv.Optional(CONF_DEACTIVATE, default=False): cv.boolean,
                cv.Optional(CONF_MIN_VALUE, default=min_val): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=max_val): cv.float_,
                cv.Optional(CONF_STEP, default=step): cv.float_,
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
                cv.GenerateID(): cv.declare_id(WR3223ParameterNumber),
            }
        )
        .extend(cv.COMPONENT_SCHEMA)
    )


# Parameter-Definitionen mit Kommandos und Grenzen
# Format: key: (name, command, min, max, step, unit, icon, requires_ewt, requires_air_balance)
PARAMETER_CONFIGS = {
    # Luftbalance (nur wenn air_balance_present=true)
    CONF_SUPPLY_AIR_DIFF: ("Zuluft Stellwert", "LD", -20, 20, 1, UNIT_PERCENT, "mdi:fan-plus", False, True),
    CONF_EXHAUST_AIR_DIFF: ("Abluft Stellwert", "Ld", -20, 20, 1, UNIT_PERCENT, "mdi:fan-minus", False, True),
    
    # EWT (nur wenn ewt_present=true)
    CONF_EWT_SUMMER: ("EWT Sommer", "ES", 15, 40, 1, UNIT_CELSIUS, "mdi:thermometer-chevron-up", True, False),
    CONF_EWT_WINTER: ("EWT Winter", "EW", -20, 10, 1, UNIT_CELSIUS, "mdi:thermometer-chevron-down", True, False),
    CONF_BRINE_PUMP_ON: ("Solepumpe Ein", "EE", -3, 5, 1, UNIT_CELSIUS, "mdi:pump", True, False),
    CONF_BRINE_PUMP_OFF: ("Solepumpe Aus", "EA", -5, 15, 1, UNIT_CELSIUS, "mdi:pump-off", True, False),
    CONF_SUMMER_STOP: ("Sommer Stopp", "Es", 5, 30, 1, UNIT_CELSIUS, "mdi:sun-thermometer", True, False),
}


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WR3223_ID): cv.use_id(WR3223),
        # **Optionen für bedingte Erstellung**
        cv.Optional(CONF_EWT_PRESENT, default=False): cv.boolean,
        cv.Optional(CONF_AIR_BALANCE_PRESENT, default=False): cv.boolean,
        
        cv.Optional(CONF_NUMBERS, default={}): cv.Schema(
            {
                cv.Optional(CONF_VENT_LEVEL_1_SPEED, default={}): _speed_schema(
                    "Luftstufe 1 Geschwindigkeit"
                ),
                cv.Optional(CONF_VENT_LEVEL_2_SPEED, default={}): _speed_schema(
                    "Luftstufe 2 Geschwindigkeit"
                ),
                cv.Optional(CONF_VENT_LEVEL_3_SPEED, default={}): _speed_schema(
                    "Luftstufe 3 Geschwindigkeit"
                ),
            }
        ),
        cv.Optional(CONF_PARAMETERS, default={}): cv.Schema(
            {
                cv.Optional(key, default={}): _parameter_schema(
                    cfg[0], cfg[1], cfg[2], cfg[3], cfg[4], cfg[5], cfg[6]
                )
                for key, cfg in PARAMETER_CONFIGS.items()
            }
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WR3223_ID])
    
    # Bedingungen prüfen
    ewt_present = config.get(CONF_EWT_PRESENT, False)
    air_balance_present = config.get(CONF_AIR_BALANCE_PRESENT, False)
    
    # Lüfterstufen (bestehender Code)
    numbers_conf = config.get(CONF_NUMBERS, {})

    async def build_speed(key: str, level: int):
        conf = numbers_conf.get(key)
        if conf is None or conf.get(CONF_DEACTIVATE):
            return
        var = cg.new_Pvariable(conf[CONF_ID], parent, level)
        await cg.register_component(var, conf)
        await number.register_number(
            var,
            conf,
            min_value=conf.get(CONF_MIN_VALUE, 40),
            max_value=conf.get(CONF_MAX_VALUE, 100),
            step=conf.get(CONF_STEP, 1),
        )

    await build_speed(CONF_VENT_LEVEL_1_SPEED, 1)
    await build_speed(CONF_VENT_LEVEL_2_SPEED, 2)
    await build_speed(CONF_VENT_LEVEL_3_SPEED, 3)

    # Parameter (mit bedingter Erstellung)
    parameters_conf = config.get(CONF_PARAMETERS, {})

    for key, (default_name, command, min_val, max_val, step, unit, icon, requires_ewt, requires_air_balance) in PARAMETER_CONFIGS.items():
        # Prüfen ob Parameter erstellt werden soll
        if requires_ewt and not ewt_present:
            continue  # EWT-Parameter überspringen wenn kein EWT
        if requires_air_balance and not air_balance_present:
            continue  # Luftbalance-Parameter überspringen wenn nicht gewünscht
        
        conf = parameters_conf.get(key)
        if conf is None or conf.get(CONF_DEACTIVATE):
            continue
        
        var = cg.new_Pvariable(conf[CONF_ID], parent, command)
        await cg.register_component(var, conf)
        await number.register_number(
            var,
            conf,
            min_value=conf.get(CONF_MIN_VALUE, min_val),
            max_value=conf.get(CONF_MAX_VALUE, max_val),
            step=conf.get(CONF_STEP, step),
        )