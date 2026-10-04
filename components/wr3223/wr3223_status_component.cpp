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

            // NEU: Einmaliges Auslesen des echten WF-Zustands nach dem Booten
            if (parent_ != nullptr && parent_->connector_ != nullptr)
            {
                ESP_LOGI(TAG, "Lese initialen Zustand für Wärmepumpen-Freigabe (WF) aus...");
                
                parent_->connector_->send_request(
                    WR3223Commands::WF,
                    [this](char *resp, bool ok)
                    {
                        if (ok && resp != nullptr)
                        {
                            std::string response_str(resp);
                            bool actual_state = (response_str == "1");
                            ESP_LOGI(TAG, "Initialer WF-Zustand erfolgreich ausgelesen: %s", response_str.c_str());
                            
                            // Aktualisiere alle registrierten Schalter mit dem echten Wert
                            for (auto *ctrl : this->controls_)
                            {
                                // Wir rufen ein erweitertes Signal auf
                                ctrl->on_status(this->holder_);
                            }
                            
                            // Trick: Um den Schalter direkt zu erreichen, publishen wir den Zustand
                            // an die UI (wird in Schritt 2 in wr3223_status_switch.h verarbeitet)
                            this->notify_controls();
                        }
                        else
                        {
                            ESP_LOGW(TAG, "Initiales Auslesen von WF fehlgeschlagen.");
                        }
                    });
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

            ESP_LOGI(TAG, "Hebe Schreibschutz auf (Sende RESETcode = 1)...");
            
            // 1. Schreibschutz aufheben (Re = 1)
            parent_->connector_->send_write_request(
                WR3223Commands::Re, "1",
                [this, state](char *re_answer, bool re_success) {
                    if (!re_success) {
                        ESP_LOGW("wr3223_status_component", "Konnte RESETcode nicht auf 1 setzen. Versuche WF trotzdem...");
                    } else {
                        ESP_LOGD("wr3223_status_component", "Konfigurationsmodus aktiv (RESETcode=1).");
                    }

                    // 2. Eigentlichen WF-Befehl senden
                    std::string data = state ? "1" : "0";
                    ESP_LOGD("wr3223_status_component", "Sende WF Befehl: %s", data.c_str());

                    this->parent_->connector_->send_write_request(
                        WR3223Commands::WF, data,
                        [this, state](char *wf_answer, bool wf_success) {
                            if (!wf_success) {
                                ESP_LOGE("wr3223_status_component", "Schreibzugriff auf WF trotz RESETcode verweigert (NAK)!");
                                
                                // Zustand in Home Assistant korrigieren (Zurückspringen auf Ist-Wert)
                                for (auto *ctrl : this->controls_) {
                                    this->write_status(); 
                                }
                            } else {
                                ESP_LOGI("wr3223_status_component", "WF erfolgreich von Wärmepumpe übernommen!");
                            }
                        });
                });
        }

    } // namespace wr3223
} // namespace esphome