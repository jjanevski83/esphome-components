#include "wr3223_vent_speed_number.h"
#include "wr3223_helper.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace wr3223
    {

        static const char *const TAG = "wr3223_vent_speed_number";

        const char *WR3223VentSpeedNumber::get_command() const
        {
            switch (level_)
            {
            case 1:
                return WR3223Commands::L1;
            case 2:
                return WR3223Commands::L2;
            case 3:
                return WR3223Commands::L3;
            case 10:
                return "Rd"; // Internes Kürzel für Raumsollwert
            case 20:
                return "SP"; // Internes Kürzel für Zuluftsoll
            default:
                return nullptr;
            }
        }

        void WR3223VentSpeedNumber::setup()
        {
            const char *cmd = get_command();
            if (cmd == nullptr || parent_ == nullptr || parent_->connector_ == nullptr)
                return;

                // DEINE ANPASSUNG: Feste Standardwerte für die Temperaturen erzwingen
            if (level_ == 10) {
                this->publish_state(21.5);
                this->control(21.5); // Schickt den Wert direkt beim Start an die Anlage
                return;
            }
            if (level_ == 20) {
                this->publish_state(20.0);
                this->control(20.0); // Schickt den Wert direkt beim Start an die Anlage
                return;
            }


            parent_->connector_->send_request(cmd, [this, cmd](char *resp, bool ok)
                                              {
                if (ok) {
                    int val = WR3223Helper::to_int(resp, true);
                    this->publish_state(val);
                } else {
                    ESP_LOGW(TAG, "Failed to read initial value for %s", cmd);
                } });
        }

        void WR3223VentSpeedNumber::control(float value)
        {
            const char *cmd = get_command();
            if (cmd == nullptr || parent_ == nullptr || parent_->connector_ == nullptr)
                return;

            std::string data;
            
            // DEINE ANPASSUNG: Floats mit einer Nachkommastelle für Temperaturen konvertieren
            if (level_ == 10 || level_ == 20) {
                // Konvertiert z.B. 21.5 zu "21.5" für das serielle Protokoll
                char buf[16];
                snprintf(buf, sizeof(buf), "%.1f", value);
                data = buf;
            } else {
                // Originaler Code für Lüfterstufen (Ganzzahlen)
                int val = static_cast<int>(value);
                data = std::to_string(val);
            }

            //int val = static_cast<int>(value);
            //std::string data = std::to_string(val);
            parent_->connector_->send_write_request(cmd, data, [this, val](char *, bool ok)
                                                    {
            ESP_LOGD(TAG, "Write %d result %d", val, ok);
            if (ok)
                this->publish_state(val); });
        }

    } // namespace wr3223
} // namespace esphome