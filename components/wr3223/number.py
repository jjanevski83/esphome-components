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
    ENTITY_CATEGORY_CONFIG,
    UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE
)

from . import WR3223, wr3223_ns, CONF_WR3223_ID, CONF_DEACTIVATE

WR3223VentSpeedNumber = wr3223_ns.class_(
    "WR3223VentSpeedNumber", number.Number, cg.Component
)

#WR3223TempSollNumber = wr3223_ns.class_(
#    "WR3223TempSollNumber", number.Number, cg.Component

CONF_NUMBERS = "numbers"
CONF_VENT_LEVEL_1_SPEED = "vent_level_1_speed"
CONF_VENT_LEVEL_2_SPEED = "vent_level_2_speed"
CONF_VENT_LEVEL_3_SPEED = "vent_level_3_speed"

# NEUE Keys für deine Temperatur-Sollwerte
CONF_RD_SOLLWERT = "rd_sollwert"
CONF_SP_SOLLWERT = "sp_sollwert"


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

# NEUES Schema speziell für deine Temperatur-Sollwerte
def _temp_soll_schema(default_name: str, min_v: float, max_v: float):
    return (
        number.number_schema(
            WR3223VentSpeedNumber,
            unit_of_measurement=UNIT_CELSIUS,
            device_class=DEVICE_CLASS_TEMPERATURE,
            icon="mdi:thermometer",
        )
        .extend(
            {
                cv.Optional(CONF_NAME, default=default_name): cv.string_strict,
                cv.Optional(CONF_DEACTIVATE, default=False): cv.boolean,
                cv.Optional(CONF_MIN_VALUE, default=min_v): cv.float_,
                cv.Optional(CONF_MAX_VALUE, default=max_v): cv.float_,
                cv.Optional(CONF_STEP, default=0.5): cv.float_,
                cv.Optional(CONF_ENTITY_CATEGORY, default=ENTITY_CATEGORY_CONFIG): cv.entity_category,
            }
        )
        .extend(cv.COMPONENT_SCHEMA)
    )



CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WR3223_ID): cv.use_id(WR3223),
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

                # Deine beiden neuen Elemente werden hier fest verankert
                cv.Optional(CONF_RD_SOLLWERT, default={}): _temp_soll_schema(
                    "Raumsollwert (Rd)", 15.0, 30.0
                ),
                cv.Optional(CONF_SP_SOLLWERT, default={}): _temp_soll_schema(
                    "Zuluftsoll Temperatur (SP)", 15.0, 40.0
                ),

            }
        ),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_WR3223_ID])
    numbers_conf = config.get(CONF_NUMBERS, {})

    async def build(key: str, level: int):
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

# 2. NEUER Builder für deine Temperatur-Sollwerte
    async def build_temp(key: str, command: str):
        conf = numbers_conf.get(key)
        if conf is None or conf.get(CONF_DEACTIVATE):
            return
        # Erzeugt das C++ Objekt und übergibt die Hauptkomponente sowie das Kürzel ("Rd" oder "SP")
        var = cg.new_Pvariable(conf[CONF_ID], parent, command)
        await cg.register_component(var, conf)
        await number.register_number(
            var,
            conf,
            min_value=conf.get(CONF_MIN_VALUE),
            max_value=conf.get(CONF_MAX_VALUE),
            step=conf.get(CONF_STEP),
        )

    await build(CONF_VENT_LEVEL_1_SPEED, 1)
    await build(CONF_VENT_LEVEL_2_SPEED, 2)
    await build(CONF_VENT_LEVEL_3_SPEED, 3)

    # Erzeugt deine neuen Temperaturregler automatisch im Code (Level 10 und 20)
    await build(CONF_RD_SOLLWERT, 10)
    await build(CONF_SP_SOLLWERT, 20)
