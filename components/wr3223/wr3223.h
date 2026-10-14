#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "wr3223_connector.h"
#include <vector>


namespace esphome
{
    namespace wr3223
    {

        class WR3223RelaisComponent; // forward declaration

        class WR3223StartUpListener
        {
        public:
            virtual void on_startup() = 0;
        };

        class WR3223 : public PollingComponent
        {
        public:
            WR3223(uart::UARTComponent *parent, uint32_t update_interval = 5000)
                : PollingComponent(update_interval) {}

            void setup() override;
            void update() override;
            void dump_config() override;

            void set_connector(WR3223Connector *connector) { connector_ = connector; }
            void set_relais_component(WR3223RelaisComponent *component) { relais_component_ = component; }

            void register_startup_listener(WR3223StartUpListener *listener) { startup_listeners_.push_back(listener); }

            void on_relais_update();

            void set_max_restore_attempts(uint8_t attempts) { max_restore_attempts_ = attempts; }

            bool is_bedienteil_aktiv();

            /// @brief liefert ob der Statup Prozess abgeschlossen wurde
            /// @return
            bool is_startup_completed() { return fresh_start_ == false; }

            WR3223Connector *connector_{nullptr};
            WR3223RelaisComponent *relais_component_{nullptr};

              // Ersetze die request_wpumpe_frei_status() Methode in deiner wr3223.h vollständig:
  void request_wpumpe_frei_status() {
    static uint8_t step_counter = 0;
    step_counter++;

    if (this->connector_ != nullptr) {
      switch (step_counter) {
        case 1:
          // === Schritt 1: Der EEPROM-Gesamtdump (DU) ===
          this->connector_->write_byte(0x04); // EOT
          this->connector_->write_str("0011"); // Adresse
          this->connector_->write_byte(0x02); // STX
          this->connector_->write_str("DU"); 
          this->connector_->write_byte(0x03); // ETX
          this->connector_->write_byte(0x11); // BCC
          this->connector_->write_byte(0x04); // EOT
          ESP_LOGD("wr3223", "Sequenzielle Diagnose: DU (EEPROM-Dump) angefordert.");
          break;

        case 2:
          // === Schritt 2: Der Relaisstatus (RL) ===
          this->connector_->write_byte(0x04); // EOT
          this->connector_->write_str("0011"); // Adresse
          this->connector_->write_byte(0x02); // STX
          this->connector_->write_str("RL"); 
          this->connector_->write_byte(0x03); // ETX
          this->connector_->write_byte(0x05); // BCC
          this->connector_->write_byte(0x04); // EOT
          ESP_LOGD("wr3223", "Sequenzielle Diagnose: RL (Relaisstatus) angefordert.");
          break;

        case 3:
          // === Schritt 3: Die digitalen Eingänge (RE) ===
          this->connector_->write_byte(0x04); // EOT
          this->connector_->write_str("0011"); // Adresse
          this->connector_->write_byte(0x02); // STX
          this->connector_->write_str("RE"); 
          this->connector_->write_byte(0x03); // ETX
          this->connector_->write_byte(0x0B); // BCC
          this->connector_->write_byte(0x04); // EOT
          ESP_LOGD("wr3223", "Sequenzielle Diagnose: RE (Eingangsstatus) angefordert.");
          step_counter = 0; // Das Karussell startet beim nächsten Takt wieder von vorn
          break;

        default:
          step_counter = 0;
          break;
      }
    }
  



        }
            


        private:
            /// @brief nach einem stromausfall oder ähnlichem, verhalten wir uns anders im
            /// ersten Update
            bool fresh_start_{true};
            uint8_t startup_counter_{0};
            uint8_t max_restore_attempts_{4};
            std::vector<WR3223StartUpListener *> startup_listeners_{};
        };

    } // namespace wr3223
} // namespace esphome
