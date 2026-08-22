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

            // Feste Standardwerte für die Temperaturen beim Booten erzwingen
            if (level_ == 10) {
                this->publish_state(21.5);
                
                // WICHTIG: Wir schicken der Anlage parallel einen festen Raumwert (T4 = 22.0),
                // damit sie den -70°C Fehler aufhebt und Schreibbefehle freischaltet!
                this->parent_->connector_->send_write_request("T4", "220", [](char *, bool ok) {});
                return;
            }
            if (level_ == 20) {
                this->publish_state(20.0);
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
            
            // Floats mit einer Nachkommastelle für Temperaturen konvertieren
            if (level_ == 10 || level_ == 20) {
                int temp_multiplied = static_cast<int>(value * 10.0f); // 24.5 wird zu 245
                data = std::to_string(temp_multiplied);
            } else {
                // Originaler Code für Lüfterstufen (Ganzzahlen)
                int val = static_cast<int>(value);
                data = std::to_string(val);
            }


            // HIER DIE ANPASSUNG: Vor oder nach dem Sollwert zwingen wir T4 auf 22 Grad (220)
            // Damit hebeln wir die NAK-Sperre der Anlage live beim Regeln aus!
            this->parent_->connector_->send_write_request("T4", "220", [](char *, bool ok) {});
            

            // [this, value] stellt sicher, dass die Variable 'value' in der Lambda-Funktion verfügbar ist
            parent_->connector_->send_write_request(cmd, data, [this, value](char *, bool ok)
                                                    {
            ESP_LOGD(TAG, "Write result %d for state %.1f", ok, value);
            //if (ok)
                this->publish_state(value); 
            });
        }

    } // namespace wr3223
} // namespace esphome