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

            // === Sende-Befehle für WPumpeFrei via nativem UART mit Zeitverriegelung ===
            void write_wpumpe_frei(bool state) {
                if (this->connector_ != nullptr) {
                // 1. Telegramm-Kopf: EOT (0x04) + Gruppenadresse "0011"
                this->connector_->write_byte(0x04);
                this->connector_->write_str("0011");
                
                // 2. Nutzdaten-Kopf: STX (0x02)
                this->connector_->write_byte(0x02);
                
                // 3. Befehl senden & passende, von uns vorberechnete BCC-Prüfsumme anhängen
                if (state) {
                    this->connector_->write_str("PA0601");
                    this->connector_->write_byte(0x03); // ETX
                    this->connector_->write_byte(0x32); // XOR-BCC für PA0601
                } else {
                    this->connector_->write_str("PA0600");
                    this->connector_->write_byte(0x03); // ETX
                    this->connector_->write_byte(0x33); // XOR-BCC für PA0600
                }
                
                // 4. Telegramm-Ende: Abschließendes EOT (0x04)
                this->connector_->write_byte(0x04);
                
                ESP_LOGD("wr3223", "Parameter WPumpeFrei via UART gesendet (State: %d).", state);
                }
            }

            void request_wpumpe_frei_status() {
                // Zeitverriegelung: Nur alle 30.000 ms (30 Sekunden) eine Anfrage zulassen,
                // um die serielle Schnittstelle und die Anlage vor Überflutung (Spam) zu schützen
                uint32_t now = millis();
                static uint32_t last_request = 0;
                if (now - last_request < 30000) {
                return; 
                }
                last_request = now;

                if (this->connector_ != nullptr) {
                // 1. Telegramm-Kopf: EOT (0x04) + Gruppenadresse "0011"
                this->connector_->write_byte(0x04);
                this->connector_->write_str("0011");
                
                // 2. Nutzdaten-Kopf: STX (0x02)
                this->connector_->write_byte(0x02);
                
                // 3. Lese-Befehl für Register 06 senden (Muss im String immer 4 Zeichen haben)
                this->connector_->write_str("PA06");
                this->connector_->write_byte(0x03); // ETX
                this->connector_->write_byte(0x33); // XOR-BCC für PA06 ('P' ^ 'A' ^ '0' ^ '6')
                
                // 4. Telegramm-Ende: Abschließendes EOT (0x04)
                this->connector_->write_byte(0x04);
                
                ESP_LOGD("wr3223", "Statusabfrage für PA06 gesendet.");
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
