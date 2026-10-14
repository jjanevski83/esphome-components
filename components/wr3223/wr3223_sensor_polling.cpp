#include "wr3223_sensor_polling.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace wr3223
    {

        static const char *const TAG = "wr3223_sensor_polling";

        void WR3223SensorPollingComponent::update()
        {
            ESP_LOGD(TAG, "Sende Anfrage für Kommando: %s", command_);

            this->parent_->connector_->send_request(command_, [this](char *response, bool success)
                                                    {
        if (success)
            this->process_response(response);
        else
            ESP_LOGW(TAG, "Command %s: Timeout", command_); });
        }

        void WR3223SensorPollingComponent::process_response(char *response)
        {
            if (!response || strlen(response) == 0)
            {
                ESP_LOGW(TAG, "Command %s: Keine Antwort vom WR3223 erhalten.", command_);
                return;
            }

            ESP_LOGI(TAG, "Command %s: Antwort = %s", command_, response);

            if (sensor_)
            {
                float value = atof(response);
                ESP_LOGD(TAG, "Sensorwert für %s: %f", command_, value);
                sensor_->publish_state(value);
            }
            else if (binary_sensor_)
            {
                bool state = (strcmp(response, "1") == 0);
                ESP_LOGD(TAG, "BinarySensor für %s: %d", command_, state);
                binary_sensor_->publish_state(state);
            }
            else if (text_sensor_)
            {
                ESP_LOGD(TAG, "TextSensor für %s: %s", command_, response);
                text_sensor_->publish_state(response);
            }

/*
*/
// === KORRIGIERTER KLARTEXT-PARSER (Nutzt command_ mit Unterstrich) ===
            std::string current_cmd(command_);
            if (current_cmd == "DU" && response != nullptr) {
                ESP_LOGI("wr3223_dump", "=== HERMES EEPROM REGISTER MAP (DU) ===");
                std::string resp_str(response);
                int reg_index = 0;
                
                for (size_t i = 0; i < resp_str.length() - 1; i += 2) {
                    std::string hex_byte = resp_str.substr(i, 2);
                    int reg_value = std::stol(hex_byte, nullptr, 16); 
                    
                    switch(reg_index) {
                        case 1:  ESP_LOGI("wr3223_dump", "  [0x01] Lüfterstufe 1 Leistung: %d %%", reg_value); break;
                        case 2:  ESP_LOGI("wr3223_dump", "  [0x02] Lüfterstufe 2 Leistung: %d %%", reg_value); break;
                        case 3:  ESP_LOGI("wr3223_dump", "  [0x03] Lüfterstufe 3 Leistung: %d %%", reg_value); break;
                        case 5:  ESP_LOGI("wr3223_dump", "  [0x05] ZuHeizFrei (Zusatzheizung): %s", (reg_value == 1 ? "FREIGEGEBEN" : "GESPERRT")); break;
                        case 6:  
                            ESP_LOGI("wr3223_dump", "  [0x06] WPumpeFREI (Wärmepumpe):    %s <--- CONFIG STATUS", (reg_value == 1 ? "FREIGEGEBEN" : "GESPERRT")); 
                            break;
                        default:
                            if (reg_index <= 15) { 
                                ESP_LOGI("wr3223_dump", "  [0x%02X] Allgemeines Register: %d (Hex: 0x%02X)", reg_index, reg_value, reg_value);
                            }
                            break;
                    }
                    reg_index++;
                }
                ESP_LOGI("wr3223_dump", "========================================");
            }
/*
*/

        }

    } // namespace wr3223
} // namespace esphome
