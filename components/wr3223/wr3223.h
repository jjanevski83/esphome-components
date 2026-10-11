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

            /*void write_wpumpe_frei(bool state) {
                if (this->connector_ != nullptr) {
                    // Konstruiert "PA0601" (Freigabe) oder "PA0600" (Sperre)
                    std::string cmd = state ? "PA0601" : "PA0600";
                    
                    // Sende das Start-Byte (STX = 0x02)
                    this->connector_->write_byte(0x02);
                    
                    // Sende den ASCII-Befehlsstring zeichenweise
                    this->connector_->write_str(cmd.c_str());
                    
                    // Sende das End-Byte (ETX = 0x03)
                    this->connector_->write_byte(0x03);
                    
                    ESP_LOGD("wr3223", "Parameter WPumpeFrei via Connector gesendet: %s", cmd.c_str());
                }
            }*/

              // Schreibt den Zustand aktiv an das EEPROM
                void write_wpumpe_frei(bool state) {
                    if (this->connector_ != nullptr) {
                    this->connector_->write_byte(0x04); // EOT (Reset)
                    this->connector_->write_str("0011"); // Gruppenadresse
                    this->connector_->write_byte(0x02); // STX
                    
                    if (state) {
                        this->connector_->write_str("PA0601");
                        this->connector_->write_byte(0x03); // ETX
                        this->connector_->write_byte(0x32); // BCC-Prüfsumme für PA0601
                    } else {
                        this->connector_->write_str("PA0600");
                        this->connector_->write_byte(0x03); // ETX
                        this->connector_->write_byte(0x33); // BCC-Prüfsumme für PA0600
                    }
                    this->connector_->write_byte(0x04); // EOT
                    }
                }

                // Fragt das Register ab unter Einhaltung des binären Hermes-Protokolls
                void request_wpumpe_frei_status() {
                    if (this->connector_ != nullptr) {
                    this->connector_->write_byte(0x04); // EOT (Reset)
                    this->connector_->write_str("0011"); // Gruppenadresse
                    this->connector_->write_byte(0x02); // STX
                    
                    // KORREKTUR: "PA" als Befehl + das Register 0x06 als rohes Byte senden!
                    this->connector_->write_str("PA"); 
                    this->connector_->write_byte(0x06); // Das rohe Binär-Byte für Register 6
                    
                    this->connector_->write_byte(0x03); // ETX
                    
                    // BCC neu berechnen: 'P' (0x50) ^ 'A' (0x41) ^ 0x06 = 0x17
                    this->connector_->write_byte(0x17); 
                    
                    this->connector_->write_byte(0x04); // EOT
                    
                    ESP_LOGD("wr3223", "Protokollkonforme Binär-Abfrage für Register PA(0x06) gesendet.");
                    }
                }


            WR3223Connector *connector_{nullptr};
            WR3223RelaisComponent *relais_component_{nullptr};

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
