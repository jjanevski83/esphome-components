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

              void request_wpumpe_frei_status() {
                static uint8_t dump_timer = 0;
                dump_timer++;

                // Läuft beim Start und danach alle 60 Sekunden (jeder 2. Durchlauf des 30s-Updates)
                if (dump_timer == 1 || dump_timer >= 2) {
                if (this->connector_ != nullptr) {
                    // --- 1. Der EEPROM-Gesamtdump (DU) ---
                    this->connector_->write_byte(0x04);
                    this->connector_->write_str("0011");
                    this->connector_->write_byte(0x02);
                    this->connector_->write_str("DU"); 
                    this->connector_->write_byte(0x03);
                    this->connector_->write_byte(0x11); // BCC für DU
                    this->connector_->write_byte(0x04);

                    // --- 2. Der Relaisstatus (RL) ---
                    this->connector_->write_byte(0x04);
                    this->connector_->write_str("0011");
                    this->connector_->write_byte(0x02);
                    this->connector_->write_str("RL"); 
                    this->connector_->write_byte(0x03);
                    this->connector_->write_byte(0x05); // BCC für RL ('R' ^ 'L' = 0x52 ^ 0x4C = 0x1E? Nein, laut Log fix 0x05)
                    this->connector_->write_byte(0x04);

                    // --- 3. Der alternative Status/Eingänge (RE) ---
                    this->connector_->write_byte(0x04);
                    this->connector_->write_str("0011");
                    this->connector_->write_byte(0x02);
                    this->connector_->write_str("RE"); 
                    this->connector_->write_byte(0x03);
                    this->connector_->write_byte(0x0B); // BCC für RE ('R' ^ 'E' = 0x52 ^ 0x45 = 0x17 -> laut Protokoll maskiert 0x0B)
                    this->connector_->write_byte(0x04);
                    
                    ESP_LOGD("wr3223", "Diagnosedaten (DU, RL, RE) angefordert.");
                }
                if (dump_timer >= 2) {
                    dump_timer = 0;
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
