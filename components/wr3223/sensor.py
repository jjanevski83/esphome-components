import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    UNIT_CELSIUS,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    CONF_DEVICE_CLASS,
    CONF_NAME,
    CONF_SENSORS,
    CONF_FRIENDLY_NAME,
    CONF_UPDATE_INTERVAL,
    CONF_UNIT_OF_MEASUREMENT,
    CONF_ACCURACY_DECIMALS,
)

# WR3223 Namespace holen (bereits in __init__.py definiert)
from . import WR3223, CONF_WR3223_ID, CONF_DEACTIVATE, wr3223_ns

# Sensor-Polling-Komponente (für alle Sensoren)
WR3223SensorPollingComponent = wr3223_ns.class_("WR3223SensorPollingComponent", cg.PollingComponent)

# Neue Konstante für EWT
CONF_EWT_PRESENT = "ewt_present"

# Liste der unterstützten Sensor-Kommandos mit ihren Standardwerten
SENSOR_COMMANDS = {
    # Temperatur-Sensoren
    "T1": ("Verdampfertemperatur", UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE),
    "T2": ("Kondensatortemperatur", UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE),
    "T3": ("Außentemperatur", UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE),
    "T5": ("Nach Wärmetauscher (Fortluft)", UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE),
    "T6": ("Zulufttemperatur", UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE),
    "T8": ("Nach Vorheizregister", UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE),
    # Drehzahl-Sensoren - WICHTIG: Keine device_class für rpm!
    "NA": ("Drehzahl Abluft", "rpm", None),
    "NZ": ("Drehzahl Zuluft", "rpm", None),
}

# EWT-abhängige Sensoren (werden nur erstellt, wenn ewt_present=true)
EWT_SENSOR_COMMANDS = {
    "T7": ("Temp nach EWT", UNIT_CELSIUS, DEVICE_CLASS_TEMPERATURE),
}

CONF_COMMAND = "command"
CONF_SENSOR_POLLING_COMPONENT_ID = "polling_component_id"
CONF_SENSORS_CUSTOM = "sensors_custom"


def validate_custom_command(value):
    """Validiert, dass benutzerdefinierte Kommandos genau 2 Zeichen haben."""
    value = cv.string(value)
    if not (len(value) == 2 and value.isalnum()):
        raise cv.Invalid(f"Custom command '{value}' must be exactly two alphanumeric characters long.")
    return value


def _sensor_schema(default_name: str, unit: str, device_class: str):
    """Hilfsfunktion zum Erstellen eines Sensor-Schemas"""
    schema_dict = {
        cv.GenerateID(CONF_SENSOR_POLLING_COMPONENT_ID): cv.declare_id(WR3223SensorPollingComponent),
        cv.Optional(CONF_DEACTIVATE, default=False): cv.boolean,
        cv.Optional(CONF_NAME, default=default_name): cv._validate_entity_name,
        cv.Optional(CONF_UNIT_OF_MEASUREMENT, default=unit): sensor.validate_unit_of_measurement,
    }
    
    # device_class nur hinzufügen, wenn nicht None
    if device_class is not None:
        schema_dict[cv.Optional(CONF_DEVICE_CLASS, default=device_class)] = sensor.validate_device_class
    
    return sensor.sensor_schema(
        state_class=STATE_CLASS_MEASUREMENT  # ← FIX: state_class für alle Sensoren
    ).extend(schema_dict).extend(cv.polling_component_schema("60s"))


# **Definition der einzelnen Temperatur-Sensoren**
CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_WR3223_ID): cv.use_id(WR3223),
        # **Option für EWT vorhanden**
        cv.Optional(CONF_EWT_PRESENT, default=False): cv.boolean,
        # **Standard-Sensoren mit IntelliSense (NUR vordefinierte Werte)**
        cv.Optional(CONF_SENSORS, default={}): cv.Schema(
            {
                cv.Optional(k, default={}): _sensor_schema(*SENSOR_COMMANDS[k])
                for k in SENSOR_COMMANDS.keys()
            }
        ),
        # **EWT-Sensoren (nur wenn EWT vorhanden, aber individuell konfigurierbar)**
        cv.Optional("T7", default={}): _sensor_schema(*EWT_SENSOR_COMMANDS["T7"]),
        # **Benutzerdefinierte Sensoren (MÜSSEN genau 2 Zeichen haben + Pflichtfelder)**
        cv.Optional(CONF_SENSORS_CUSTOM, default=[]): cv.ensure_list(
            sensor.sensor_schema(state_class=STATE_CLASS_MEASUREMENT)
            .extend(
                {
                    cv.GenerateID(CONF_SENSOR_POLLING_COMPONENT_ID): cv.declare_id(WR3223SensorPollingComponent),
                    cv.Required(CONF_COMMAND): cv.All(cv.string, validate_custom_command),
                    cv.Required(CONF_NAME): cv._validate_entity_name,
                    cv.Optional(CONF_UNIT_OF_MEASUREMENT): sensor.validate_unit_of_measurement,
                    cv.Optional(CONF_DEVICE_CLASS): sensor.validate_device_class,
                }
            )
            .extend(cv.polling_component_schema("60s")),
        ),
    }
).extend(cv.COMPONENT_SCHEMA)


async def generate_sensor_code(parent, sensor_config):
    """Erstellt den Code für einen einzelnen Sensor."""

    command = sensor_config[CONF_COMMAND]

    sens = await sensor.new_sensor(sensor_config)

    var = cg.new_Pvariable(
        sensor_config[CONF_SENSOR_POLLING_COMPONENT_ID],
        parent,
        sensor_config[CONF_UPDATE_INTERVAL],
        sens,
        command,
    )

    await cg.register_component(var, sensor_config)


async def to_code(config):
    """ESPHome Code-Generierung für WR3223 Sensoren"""

    # WR3223 Hauptkomponente abrufen
    parent = await cg.get_variable(config[CONF_WR3223_ID])

    # EWT vorhanden prüfen
    ewt_present = config.get(CONF_EWT_PRESENT, False)

    # Standard-Sensoren (Dictionary: {command: sensor_config})
    for command, sensor_config in config.get(CONF_SENSORS, {}).items():
        if sensor_config.get(CONF_DEACTIVATE, False):
            continue  # Sensor nicht erstellen, wenn deaktiviert

        sensor_config[CONF_COMMAND] = command
        sensor_config.setdefault(CONF_ACCURACY_DECIMALS, 1)

        await generate_sensor_code(parent, sensor_config)

    # T7 Sensor nur erstellen, wenn EWT vorhanden ist
    t7_config = config.get("T7", {})
    if ewt_present and not t7_config.get(CONF_DEACTIVATE, False):
        t7_config[CONF_COMMAND] = "T7"
        t7_config.setdefault(CONF_ACCURACY_DECIMALS, 1)
        await generate_sensor_code(parent, t7_config)

    # Benutzerdefinierte Sensoren durchgehen
    for sensor_config in config.get(CONF_SENSORS_CUSTOM, []):
        await generate_sensor_code(parent, sensor_config)