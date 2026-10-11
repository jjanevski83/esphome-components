#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "wr3223_status_component.h"

namespace esphome
{
    namespace wr3223
    {

        class WR3223StatusSwitch : public switch_::Switch,
                                   public Component,
                                   public WR3223StatusControl
        {
        public:
            void set_status_component(WR3223StatusComponent *status) { status_ = status; }

            void setup() override;
            void write_state(bool state) override;
            void on_status(WR3223StatusValueHolder *holder) override;

        protected:
            WR3223StatusComponent *status_{nullptr};
            virtual void apply_state(WR3223StatusValueHolder *holder, bool state) = 0;
            virtual bool current_state(WR3223StatusValueHolder *holder) = 0;
        };

        class WR3223HeatPumpSwitch : public WR3223StatusSwitch
        {
        protected:
            void apply_state(WR3223StatusValueHolder *holder, bool state) override
            {
                holder->setHeatPumpOn(state);
            }
            bool current_state(WR3223StatusValueHolder *holder) override
            {
                return holder->getHeatPumpOnStatus();
            }
        };

        class WR3223AdditionalHeatingSwitch : public WR3223StatusSwitch
        {
        protected:
            void apply_state(WR3223StatusValueHolder *holder, bool state) override
            {
                holder->setAdditionalHeatingOn(state);
            }
            bool current_state(WR3223StatusValueHolder *holder) override
            {
                return holder->getAdditionalHeatingOnStatus();
            }
        };

        class WR3223CoolingSwitch : public WR3223StatusSwitch
        {
        protected:
            void apply_state(WR3223StatusValueHolder *holder, bool state) override
            {
                holder->setCoolingOn(state);
            }
            bool current_state(WR3223StatusValueHolder *holder) override
            {
                return holder->getCoolingOnStatus();
            }
        };

        /*class WR3223WPumpeFreiParamSwitch : public switch_::Switch, public Component
        {
        public:
            void set_wr3223_parent(WR3223 *parent) { parent_ = parent; }

            void setup() override {
                // Initialer Zustand im Home Assistant (optimistisch/false als Standard)
                this->publish_state(false); 
            }

            void write_state(bool state) override {
                if (parent_ != nullptr) {
                    parent_->write_wpumpe_frei(state);
                }
                this->publish_state(state);
            }

        protected:
            WR3223 *parent_{nullptr};
        };*/

        class WR3223WPumpeFreiParamSwitch : public switch_::Switch,
                                           public Component
        {
        public:
            void set_wr3223_parent(WR3223 *parent) { parent_ = parent; }

            void setup() override {
                // Suche in der App nach dem automatisch erstellten PA06 Sensor
                for (auto *sens : App.get_sensors()) {
                    // Der Sensor-Name oder die ID wird vom Python-Framework vergeben
                    // Wir lauschen auf jede Zustandsänderung des PA06-Status-Sensors
                    if (sens->get_name() == "Wärmepumpe EEPROM Status") {
                        sens->add_on_state_callback([this](float value) {
                            // Wenn der Sensor 1.0 liefert, ist die WP frei (true)
                            this->publish_state(value >= 1.0f);
                        });
                        break;
                    }
                }
            }

            void write_state(bool state) override {
                if (parent_ != nullptr) {
                    parent_->write_wpumpe_frei(state);
                }
                this->publish_state(state);
            }
        };

    } // namespace wr3223
} // namespace esphome