#include "wr3223.h"
#include "wr3223_relais_component.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace wr3223
    {
        static const char *const TAG = "wr3223";

        void WR3223::setup()
        {
            ESP_LOGI(TAG, "WR3223 Hauptkomponente Setup gestartet!");
        }

        void WR3223::update()
        {                        
            if (fresh_start_ && relais_component_ != nullptr)
            {
                ESP_LOGI(TAG, "WR3223 Hauptkomponente FreshStart im Update wird ausgeführt!");
                relais_component_->update();                
            }
        }

        void WR3223::dump_config()
        {
            ESP_LOGCONFIG(TAG, "WR3223 Konfiguration:");
            ESP_LOGCONFIG(TAG, "  - Update Intervall: %d ms", this->get_update_interval());
        }

        void WR3223::on_relais_update()
        {
            if (!fresh_start_)
                return;

            startup_counter_++;
            bool bd_active = is_bedienteil_aktiv();
            ESP_LOGD(TAG, "Relais update %u/%u bedienteil=%d", startup_counter_, max_restore_attempts_, bd_active);

            if (!bd_active)
            {
                ESP_LOGD(TAG, "Startup conditions met - notifying listeners");
                for (auto *listener : startup_listeners_)
                {
                    if (listener != nullptr)
                        listener->on_startup();
                }
                fresh_start_ = false;
            }
            else if (startup_counter_ >= max_restore_attempts_)
            {
                ESP_LOGW(TAG, "Startup failed after %u attempts", startup_counter_);
                fresh_start_ = false;
            }
        }

        bool WR3223::is_bedienteil_aktiv()
        {
            if (relais_component_ != nullptr)
                return relais_component_->is_bedienteil_aktiv();

            return true; // haben wir keinen Zugriff auf die RelaisComponent, so gilt der Schreibschutz
        }

        void WR3223Component::parse_line(const std::string &line) {
          // Sucht nach der Antwort von der Anlage, z.B. "Rd: 21.5" oder "Rd 21"
          if (line.rfind("Rd", 0) == 0) {
            size_t colon_pos = line.find_first_of(": ");
            if (colon_pos != std::string::npos) {
              std::string val_str = line.substr(colon_pos + 1);
              float current_soll = std::stof(val_str);
      
              // Übergibt den gelesenen Wert an Home Assistant ohne Trigger-Schleife
              if (this->raumsollwert_number_ != nullptr && this->raumsollwert_number_->state != current_soll) {
                this->raumsollwert_number_->publish_state(current_soll);
              }
            }
            return;
          }


          void WR3223Component::write_raumsollwert(float value) {
            // Wandelt den Float (z.B. 21.5) in einen String um (WR3223 erwartet oft Ganzzahlen, sonst ".0" abschneiden)
            int int_val = (int)value; 
  
            std::string cmd = "Rd " + std::to_string(int_val) + "\r\n"; // \r\n terminiert serielle Befehle
  
            // Befehl über den UART-Bus an die Hauptplatine senden
            this->write_str(cmd.c_str());
            ESP_LOGD("wr3223", "Gesendeter Raumsollwert an Anlage: %s", cmd.c_str());
          }


    } // namespace wr3223
} // namespace esphome
