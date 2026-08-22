#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "wr3223_connector.h"
#include <vector>

namespace esphome
{
    namespace wr3223
    {

        class WR3223Component : public Component, public uart::UARTDevice {
          public:
            void setup() override;
            void loop() override;
            void update();

            // Setter für die ESPHome Number-Komponente
            void set_raumsollwert_number(number::Number *num) { this->raumsollwert_number_ = num; }
  
            // Methode, die aufgerufen wird, wenn der Schieberegler in HA bewegt wird
            void write_raumsollwert(float value);

           protected:
            void parse_line(const std::string &line);
            number::Number *raumsollwert_number_{nullptr};
        };
        
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
