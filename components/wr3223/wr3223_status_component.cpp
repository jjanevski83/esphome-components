#include "wr3223_status_component.h"
#include "esphome/core/log.h"
#include "wr3223_helper.h"
#include "wr3223_connector.h"

namespace esphome
{
    namespace wr3223
    {

        static const char *const TAG = "wr3223_status_component";

        void WR3223StatusComponent::setup()
        {
            if (parent_ != nullptr)
                parent_->register_startup_listener(this);

            if (holder_ != nullptr)
                holder_->restore_state_sw();

            notify_controls();
        }

        void WR3223StatusComponent::update()
        {
            // mit den regulären Updates warten wir bis das Startup abgeschlossen wurde
            if (parent_->is_startup_completed())
                write_status();
        }

        void WR3223StatusComponent::on_startup()
        {
            if (holder_ != nullptr && !parent_->is_bedienteil_aktiv())
            {
                holder_->restore_state_sw();                
                write_status();                
            }
        }

        void WR3223StatusComponent::write_status()
        {
            if (parent_ == nullptr || parent_->connector_ == nullptr ||
                holder_ == nullptr)
                return;

            if (parent_->is_bedienteil_aktiv())
            {
                ESP_LOGW(TAG, "Bedienteil aktiv - Schreiben nicht moeglich, lese Status.");
                parent_->connector_->send_request(
                    WR3223Commands::SW,
                    [this](char *resp, bool ok)
                    {
                        ESP_LOGD(TAG, "Status readback: %s success=%d", resp, ok);
                        if (ok)
                        {
                            holder_->setSWStatus(resp);
                            notify_controls();
                        }
                    });
                return;
            }

            std::string data = std::to_string(holder_->getSwStatus());
            parent_->connector_->send_write_request(
                WR3223Commands::SW, data,
                [this](char *answer, bool success)
                {
                    ESP_LOGD(TAG, "Status write response: %s success=%d", answer, success);
                    if (!success) // bei misserfolg schreiben wir den echten Wert zurueck
                    {
                        parent_->connector_->send_request(
                            WR3223Commands::SW,
                            [this](char *resp, bool ok)
                            {
                                ESP_LOGD(TAG, "Status readback: %s success=%d", resp, ok);
                                if (ok)
                                {
                                    holder_->setSWStatus(resp);
                                    notify_controls();
                                }
                            });
                    }
                    else
                    {
                        notify_controls();
                    }
                });
        }

        void WR3223StatusComponent::notify_controls()
        {
            for (auto *ctrl : controls_)
            {
                if (ctrl != nullptr)
                    ctrl->on_status(holder_);
            }
        }

        void WR3223StatusComponent::write_wp_frei(bool state)
        {
            if (parent_ == nullptr || parent_->connector_ == nullptr)
                return;

            if (parent_->is_bedienteil_aktiv())
            {
                ESP_LOGW(TAG, "Bedienteil aktiv - WF Schreiben nicht moeglich.");
                return;
            }

            std::string data = state ? "1" : "0";
            ESP_LOGD(TAG, "Sende WF Befehl: %s", data.c_str());

            parent_->connector_->send_write_request(
                WR3223Commands::WF, data,
                [](char *answer, bool success)
                {
                    ESP_LOGD("wr3223_status_component", "WF Befehl Antwort: %s success=%d", answer, success);
                });
        }

    } // namespace wr3223
} // namespace esphome