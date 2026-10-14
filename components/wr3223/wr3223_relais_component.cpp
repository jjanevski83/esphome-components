#include "wr3223_relais_component.h"
#include "wr3223.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace wr3223
  {

    static const char *TAG = "wr3223_relais";

    void WR3223RelaisComponent::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up WR3223 Relais Component...");
    }

    void WR3223RelaisComponent::update()
    {
      if (parent_ == nullptr || parent_->connector_ == nullptr)
      {
        ESP_LOGE(TAG, "Kein gültiger WR3223-Connector gefunden!");
        return;
      }

      parent_->connector_->send_request(
          WR3223Commands::RL,
          [this](char *response, bool success)
          {
            if (success)
              this->process_response(response);
            else
              ESP_LOGW(TAG, "Relais Kommando Timeout");
          });

      ESP_LOGD(TAG, "Relais-Status-Anfrage gesendet.");
    }

    /// @brief Verarbeitet die Antwort der Steuerung.
    /// @param response Antwort-String mit den reinen Datenbits.
    void WR3223RelaisComponent::process_response(char *response)
    {
      if (!response || strlen(response) == 0)
      {
        ESP_LOGE(TAG, "Antwort ist leer oder NULL!");
        return;
      }

/*
*/
      int relais_status = std::stoi(response); // Direkt die Datenbits als Integer interpretieren

      // ===================================================================
      // KLARTEXT-LOGGER FÜR DEN AKTUELLEN RELAISSTATUS (RL)
      // ===================================================================
      ESP_LOGI("wr3223_relais", "=== HERMES ANLAGEN- & RELAISSTATUS (RL) ===");
      ESP_LOGI("wr3223_relais", "  Rohwert vom Bus: %d", relais_status);
      ESP_LOGI("wr3223_relais", "  - WP-Kompressor:   %s", ((relais_status & 1)   ? "AN (Relais geschlossen)" : "AUS"));
      ESP_LOGI("wr3223_relais", "  - Zusatzheizung:   %s", ((relais_status & 2)   ? "AN" : "AUS"));
      ESP_LOGI("wr3223_relais", "  - Erdwärmetauscher:%s", ((relais_status & 4)   ? "OFFEN" : "GESCHLOSSEN"));
      ESP_LOGI("wr3223_relais", "  - Sommer-Bypass:   %s", ((relais_status & 8)   ? "GEÖFFNET" : "GESCHLOSSEN"));
      ESP_LOGI("wr3223_relais", "  - Vorheizregister: %s", ((relais_status & 16)  ? "AN" : "AUS"));
      ESP_LOGI("wr3223_relais", "  - WL2-Bedienteil:  %s", ((relais_status & 64)  ? "ONLINE" : "OFFLINE / ABGEKLEMMT"));
      // Hier maskieren wir Bit 7 (128) und Bit 8 (256) für die RS232-Kontrolle aus (384 = 128 + 256)
      ESP_LOGI("wr3223_relais", "  - RS232-Modus:     %s", ((relais_status & 128) ? "MASTER (ESP Steuerung)" : "NUR LESEN"));

      if (relais_status == 384) {
          ESP_LOGI("wr3223_relais", "  -> Status-Info: ESP32 steuert die Anlage autark und fehlerfrei.");
      }
      ESP_LOGI("wr3223_relais", "===========================================");

      ESP_LOGD(TAG, "Erhaltener Relais-Status: %d", relais_status);

      // === KLARTEXT-PARSER FÜR DIGITALE EINGÄNGE (RE) ===
// Falls die Komponente zukünftig auch RE verarbeitet, liest dieser Block die Sicherheitskette aus:
int input_status = std::stoi(response); 

ESP_LOGI("wr3223_inputs", "=== HERMES DIGITALE PLATINEN-EINGÄNGE (RE) ===");
ESP_LOGI("wr3223_inputs", "  - Störung Niederdruck:       %s", ((input_status & 1)   ? "JA (Kältekreis-Fehler!)" : "NEIN (OK)"));
ESP_LOGI("wr3223_inputs", "  - Störung Hochdruck:         %s", ((input_status & 2)   ? "JA (Kältekreis-Fehler!)" : "NEIN (OK)"));
ESP_LOGI("wr3223_inputs", "  - Frostschutz-Thermostat:    %s", ((input_status & 4)   ? "AUSGELÖST!" : "OK"));
ESP_LOGI("wr3223_inputs", "  - Kondensatpumpe / Alarm:    %s", ((input_status & 8)   ? "ALARM (Voll)" : "OK"));
ESP_LOGI("wr3223_inputs", "  - Externer Freigabekontakt:  %s", ((input_status & 16)  ? "OFFEN (Sperre)" : "GESCHLOSSEN (Frei)"));
ESP_LOGI("wr3223_inputs", "  - Filterüberwachung / Timer: %s", ((input_status & 32)  ? "FILTER WECHSELN!" : "OK"));
ESP_LOGI("wr3223_inputs", "==============================================");
      
/*
      int relais_status = std::stoi(response); // Direkt die Datenbits als Integer interpretieren
      ESP_LOGD(TAG, "Erhaltener Relais-Status: %d", relais_status);
*/
      // update flag for 'bedienteil aktiv'
      bedienteil_aktiv_ = (relais_status & 64) != 0;
      ESP_LOGD(TAG, "Bedienteil aktiv: %d", bedienteil_aktiv_);

      // Sensoren anhand der Bit-Flags setzen
      for (auto it = relais_sensors_.begin(); it != relais_sensors_.end(); ++it)
      {
        int flag = it->first;
        auto sensor = it->second;
        bool state = (relais_status & flag) != 0;
        sensor->publish_state(state);
      }

      if (parent_ != nullptr)
        parent_->on_relais_update();
    }

    void WR3223RelaisComponent::register_relais_sensor(
        int flag, binary_sensor::BinarySensor *sensor)
    {
      this->relais_sensors_[flag] = sensor;
    }

  } // namespace wr3223
} // namespace esphome
