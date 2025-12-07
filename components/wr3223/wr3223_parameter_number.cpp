#include "wr3223_parameter_number.h"
#include "wr3223_helper.h"
#include "esphome/core/log.h"

namespace esphome
{
    namespace wr3223
    {

        static const char *const TAG = "wr3223_parameter_number";

        void WR3223ParameterNumber::setup()
        {
            if (parent_ == nullptr || parent_->connector_ == nullptr)
            {
                ESP_LOGE(TAG, "Parent or connector is null");
                return;
            }

            // Initialen Wert von der Anlage auslesen
            parent_->connector_->send_request(command_, [this](char *resp, bool ok)
            {
                if (ok) {
                    float val = WR3223Helper::to_float(resp);
                    this->publish_state(val);
                    ESP_LOGD(TAG, "Initial value for %s: %.1f", command_, val);
                } else {
                    ESP_LOGW(TAG, "Failed to read initial value for %s", command_);
                }
            });
        }

        void WR3223ParameterNumber::control(float value)
        {
            if (parent_ == nullptr || parent_->connector_ == nullptr)
            {
                ESP_LOGE(TAG, "Parent or connector is null");
                return;
            }

            // Wert als String formatieren
            std::string data = WR3223Helper::to_string(value, 1);
            
            ESP_LOGD(TAG, "Writing %s = %s", command_, data.c_str());

            parent_->connector_->send_write_request(command_, data, [this, value](char *resp, bool ok)
            {
                if (ok) {
                    this->publish_state(value);
                    ESP_LOGD(TAG, "Successfully wrote %s = %.1f", command_, value);
                } else {
                    ESP_LOGW(TAG, "Failed to write %s = %.1f", command_, value);
                    // Bei Fehler aktuellen Wert neu auslesen
                    parent_->connector_->send_request(command_, [this](char *resp, bool success)
                    {
                        if (success) {
                            float val = WR3223Helper::to_float(resp);
                            this->publish_state(val);
                        }
                    });
                }
            });
        }

    } // namespace wr3223
} // namespace esphome