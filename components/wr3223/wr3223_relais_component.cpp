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

                if (command == "DU") {
                  ESP_LOGI("wr3223_dump", "=== HERMES EEPROM REGISTER MAP (DU) ===");
                  std::string resp_str(response);
                  int reg_index = 0;
                  
                  // Wir laufen zeichenweise in 2er-Schritten durch die Hex-Antwort
                  for (size_t i = 0; i < resp_str.length() - 1; i += 2) {
                      std::string hex_byte = resp_str.substr(i, 2);
                      int reg_value = std::stol(hex_byte, nullptr, 16); 
                      
                      switch(reg_index) {
                          case 1:  ESP_LOGI("wr3223_dump", "  [0x01] Lüfterstufe 1 Leistung: %d %%", reg_value); break;
                          case 2:  ESP_LOGI("wr3223_dump", "  [0x02] Lüfterstufe 2 Leistung: %d %%", reg_value); break;
                          case 3:  ESP_LOGI("wr3223_dump", "  [0x03] Lüfterstufe 3 Leistung: %d %%", reg_value); break;
                          case 5:  ESP_LOGI("wr3223_dump", "  [0x05] ZuHeizFrei (Zusatzheizung): %s", (reg_value == 1 ? "FREIGEGEBEN" : "GESPERRT")); break;
                          case 6:  
                              ESP_LOGI("wr3223_dump", "  [0x06] WPumpeFREI (Wärmepumpe):    %s <--- EEPROM STATUS", (reg_value == 1 ? "FREIGEGEBEN" : "GESPERRT")); 
                              
                              // Synchronisiert den echten EEPROM-Wert mit der Switch-Sicherheitslogik auf dem ESP32
                              if (this->get_holder() != nullptr) {
                                  this->get_holder()->setWPumpeFreiLogik(reg_value == 1);
                              }
                              break;
                          default:
                              if (reg_index <= 15) { 
                                  ESP_LOGI("wr3223_dump", "  [0x%02X] Allgemeines Register: %d (Hex: 0x%02X)", reg_index, reg_value, reg_value);
                              }
                              break;
                      }
                      reg_index++;
                  }
                  ESP_LOGI("wr3223_dump", "========================================");
              }

              // ===================================================================
              // 2. PARSER FÜR DAS KOMMANDO: RL (Aktiver Relaisstatus)
              // ===================================================================
              if (command == "RL") {
                  int status = WR3223Helper::to_int(response, true);
                  ESP_LOGI("wr3223_relais", "=== HERMES ANLAGEN- & RELAISSTATUS (RL) ===");
                  ESP_LOGI("wr3223_relais", "  Rohwert vom Bus: %d", status);
                  ESP_LOGI("wr3223_relais", "  - WP-Kompressor:   %s", ((status & 1)   ? "AN (Relais geschlossen)" : "AUS"));
                  ESP_LOGI("wr3223_relais", "  - Zusatzheizung:   %s", ((status & 2)   ? "AN" : "AUS"));
                  ESP_LOGI("wr3223_relais", "  - Erdwärmetauscher:%s", ((status & 4)   ? "OFFEN" : "GESCHLOSSEN"));
                  ESP_LOGI("wr3223_relais", "  - Sommer-Bypass:   %s", ((status & 8)   ? "GEÖFFNET" : "GESCHLOSSEN"));
                  ESP_LOGI("wr3223_relais", "  - Vorheizregister: %s", ((status & 16)  ? "AN" : "AUS"));
                  ESP_LOGI("wr3223_relais", "  - WL2-Bedienteil:  %s", ((status & 64)  ? "ONLINE" : "OFFLINE / ABGEKLEMMT"));
                  ESP_LOGI("wr3223_relais", "  - RS232-Modus:     %s", ((status & 128) ? "MASTER (ESP Steuerung)" : "NUR LESEN"));
                  ESP_LOGI("wr3223_relais", "===========================================");
              }

              // ===================================================================
              // 3. PARSER FÜR DAS KOMMANDO: RE (Digitale Eingänge / Sicherheit)
              // ===================================================================
              if (command == "RE") {
                  int status = WR3223Helper::to_int(response, true);
                  ESP_LOGI("wr3223_inputs", "=== HERMES DIGITALE EINGÄNGE (RE) ===");
                  ESP_LOGI("wr3223_inputs", "  Rohwert: %d", status);
                  ESP_LOGI("wr3223_inputs", "  - Störung Niederdruck:       %s", ((status & 1)   ? "JA (Fehler!)" : "NEIN (OK)"));
                  ESP_LOGI("wr3223_inputs", "  - Störung Hochdruck:         %s", ((status & 2)   ? "JA (Fehler!)" : "NEIN (OK)"));
                  ESP_LOGI("wr3223_inputs", "  - Frostschutz-Thermostat:    %s", ((status & 4)   ? "AUSGELÖST!" : "OK"));
                  ESP_LOGI("wr3223_inputs", "  - Kondensatpumpe / Alarm:    %s", ((status & 8)   ? "ALARM (Voll)" : "OK"));
                  ESP_LOGI("wr3223_inputs", "  - Externer Freigabekontakt:  %s", ((status & 16)  ? "OFFEN (Sperre)" : "GESCHLOSSEN (Frei)"));
                  ESP_LOGI("wr3223_inputs", "  - Filterüberwachung / Timer: %s", ((status & 32)  ? "FILTER WECHSELN!" : "OK"));
                  ESP_LOGI("wr3223_inputs", "======================================");
              }

              // ===================================================================
              // 4. PARSER FÜR DAS KOMMANDO: ER (Fehlerregister)
              // ===================================================================
              if (command == "ER") {
                  int error_code = WR3223Helper::to_int(response, true);
                  ESP_LOGI("wr3223_error", "=== HERMES FEHLERREGISTER-ANALYSE (ER) ===");
                  if (error_code == 0) {
                      ESP_LOGI("wr3223_error", "  -> Status: ALLES OK - Keine anstehenden Hardware-Fehler.");
                  } else {
                      ESP_LOGW("wr3223_error", "  Achtung! Folgende Störungen liegen aktiv an:");
                      if (error_code & 1)   ESP_LOGE("wr3223_error", "    [Bit 0] Fehler: Zuluftventilator blockiert / Drehzahlfehler");
                      if (error_code & 2)   ESP_LOGE("wr3223_error", "    [Bit 1] Fehler: Abluftventilator blockiert / Drehzahlfehler");
                      if (error_code & 4)   ESP_LOGE("wr3223_error", "    [Bit 2] ALARM: Frostschutz aktiv (Vereisungsgefahr!)");
                      if (error_code & 8)   ESP_LOGE("wr3223_error", "    [Bit 3] Fehler: Sensor-Defekt / Kurzschluss an T1-T8");
                      if (error_code & 16)  ESP_LOGE("wr3223_error", "    [Bit 4] ALARM: Pressostat Kältekreis ausgelöst (Hoch-/Niederdruck)");
                      if (error_code & 32)  ESP_LOGW("wr3223_error", "    [Bit 5] Hinweis: Filterlaufzeit abgelaufen");
                  }
                  ESP_LOGI("wr3223_error", "==========================================");
              }

      int relais_status = std::stoi(response); // Direkt die Datenbits als Integer interpretieren
      ESP_LOGD(TAG, "Erhaltener Relais-Status: %d", relais_status);

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
