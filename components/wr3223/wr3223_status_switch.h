#pragma once

#include "esphome/components/switch/switch.h"
#include "esphome/core/component.h"
#include "wr3223_status_component.h"
#include "wr3223_constants.h" // NEU: Damit WR3223Commands und WF bekannt sind

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

        class WR3223WpFreiSwitch : public switch_::Switch, public Component, public WR3223StatusControl
        {
        public:
            void set_status_component(WR3223StatusComponent *status) { 
                status_ = status; 
                if (status_ != nullptr) {
                    status_->register_status_control(this);
                }
            }
            
            void write_state(bool state) override {
                if (status_ != nullptr) {
                    status_->write_wp_frei(state);
                }
                this->publish_state(state);
            }

            // Falls die Komponente den Status aktualisiert, spiegeln wir ihn hier
            void on_status(WR3223StatusValueHolder *holder) override {
                // Hinweis: Da WF nicht im SW-Byte liegt, dient dies primär dem 
                // Zurücksetzen der UI bei Fehlern via write_status()
            }

        protected:
            WR3223StatusComponent *status_{nullptr};
        };

    } // namespace wr3223
} // namespace esphome
