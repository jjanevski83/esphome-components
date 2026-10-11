#include "wr3223_status_switch.h"
#include "esphome/core/log.h"
#include "esphome/components/sensor/sensor.h" 

namespace esphome
{
    namespace wr3223
    {

        static const char *const TAG = "wr3223_status_switch";

        void WR3223StatusSwitch::setup()
        {
            if (status_ != nullptr)
            {
                status_->register_status_control(this);
                auto holder = status_->get_holder();
                if (holder != nullptr)
                    this->publish_state(current_state(holder));
            }
        }

        void WR3223StatusSwitch::write_state(bool state)
        {
            if (status_ != nullptr)
            {
                auto holder = status_->get_holder();
                if (holder != nullptr)
                {
                    apply_state(holder, state);
                    status_->write_status();
                }
            }
            this->publish_state(state);
        }

        void WR3223StatusSwitch::on_status(WR3223StatusValueHolder *holder)
        {
            if (holder != nullptr)
            {
                this->publish_state(current_state(holder));
            }
        }

        void WR3223WPumpeFreiParamSwitch::setup() {
            if (this->status_sensor_ != nullptr) {
                this->status_sensor_->add_on_state_callback([this](float value) {
                    // Wenn der Polling-Sensor 1.0 (oder höher) liefert, ist die WP frei (true)
                    this->publish_state(value >= 1.0f);
                });
            }
        }

    } // namespace wr3223
} // namespace esphome