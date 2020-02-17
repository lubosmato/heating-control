#include <array>
#include <cmath>
#include <iostream>
#include <tuple>

#include "ds18b20.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "owb.h"

constexpr uint8_t GPIO_DS18B20_0 = 22;
constexpr size_t MAX_DEVICES = 8;

void mainTask(void *pvParameter) {
    esp_log_level_set("*", ESP_LOG_INFO);
    gpio_pullup_en(gpio_num_t(GPIO_DS18B20_0));

    owb_rmt_driver_info rmt_driver_info;
    OneWireBus *bus = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(bus, true);

    OneWireBus_ROMCode romCode;
    owb_read_rom(bus, &romCode);
    esp_log_buffer_hex("rom", romCode.bytes, 8);

    owb_write_byte(bus, OWB_ROM_MATCH);
    owb_write_rom_code(bus, romCode);
    owb_write_byte(bus, 0x44);
    vTaskDelay(500 / portTICK_RATE_MS);

    owb_write_byte(bus, OWB_ROM_MATCH);
    owb_write_rom_code(bus, romCode);
    owb_write_byte(bus, 0xbe);

    std::array<uint8_t, 9> buff;
    owb_read_bytes(bus, buff.data(), buff.size());
    esp_log_buffer_hex("app", buff.data(), buff.size());

    vTaskDelay(5000 / portTICK_RATE_MS);
    esp_restart();
    /*
    std::cout << "Hello there" << std::endl;
    for (int i = 10; i >= 0; i--) {
        std::cout << "Restarting in " << i << " seconds..." << std::endl;
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    std::cout << "Restarting now." << std::endl;
    fflush(stdout);
    esp_restart();
    */
}

class DS18B20 {
   public:
    static constexpr float ERROR_TEMPERATURE = -273.15f;
    enum Resolution : uint8_t {
        Bits9 = 1,
        Bits10,
        Bits11,
        Bits12
    };

   private:
    static constexpr float CONVERSION_TIME = 93.75f;  // [ms]
    static constexpr uint8_t ADDRESS_WRITE_SCRATCHPAD = 0x4e;
    static constexpr uint8_t ADDRESS_READ_SCRATCHPAD = 0xbe;
    static constexpr uint8_t ADDRESS_CONVERT = 0x44;
    struct Scratchpad {
        uint8_t temperature[2];  // [0] is LSB, [1] is MSB
        uint8_t trigger_high;
        uint8_t trigger_low;
        uint8_t configuration;
        uint8_t reserved[3];
        uint8_t crc;
    };

    OneWireBus &_bus;
    Resolution _resolution;

   public:
    DS18B20(OneWireBus &bus, Resolution resolution)
        : _bus(bus), _resolution(resolution) {}

    bool initialize() {
        owb_use_crc(&_bus, true);
        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "CRC on\n");
        // auto [scratchpad, success] = readScratchpad();
        auto noCpp17 = readScratchpad();
        auto scratchpad = std::get<0>(noCpp17);
        auto success = std::get<1>(noCpp17);
        if (!success) {
            return false;
        }
        constexpr uint8_t resolutionMask = 0b01100000;
        scratchpad.configuration &= ~resolutionMask;
        scratchpad.configuration |= uint8_t(_resolution) << 5;
        writeScratchpad(scratchpad);
        return true;
    }

   private:
    std::tuple<Scratchpad, bool> readScratchpad() {
        Scratchpad scratchpad;
        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "readScratchpad\n");
        owb_write_byte(&_bus, OWB_ROM_SKIP);
        owb_write_byte(&_bus, ADDRESS_READ_SCRATCHPAD);
        uint8_t *rawScratchpad = reinterpret_cast<uint8_t *>(&scratchpad);
        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "rawScratchpad\n");
        owb_read_bytes(&_bus, rawScratchpad, sizeof(Scratchpad));

        /*
        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "crcCheck\n");
        if (owb_crc8_bytes(0, rawScratchpad, sizeof(Scratchpad)) != 0) {
            esp_log_write(esp_log_level_t::ESP_LOG_ERROR, "crc", "CRC error :(\n");
            return std::tuple<Scratchpad, bool>{scratchpad, false};
        }
        */

        return std::tuple<Scratchpad, bool>{scratchpad, true};
    }

    void writeScratchpad(const Scratchpad &scratchpad) {
        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "writeScratchpad\n");
        owb_write_byte(&_bus, OWB_ROM_SKIP);
        owb_write_byte(&_bus, ADDRESS_WRITE_SCRATCHPAD);
        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "written to owb\n");
        owb_write_bytes(&_bus, const_cast<uint8_t *>(&scratchpad.trigger_high), 3);
        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "after const cast\n");
    }

   public:
    float readTemperature() {
        bool isPresent = false;
        owb_reset(&_bus, &isPresent);
        owb_write_byte(&_bus, OWB_ROM_SKIP);
        owb_write_byte(&_bus, ADDRESS_CONVERT);

        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "converting\n");
        const float conversionTime = CONVERSION_TIME * (1 << (uint8_t(_resolution) - 1));
        const int waitTime = std::ceil(conversionTime) + portTICK_RATE_MS;
        printf("waiting for %d ms\n", waitTime);
        vTaskDelay(waitTime / portTICK_RATE_MS);

        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "reading temp\n");
        // auto [scratchpad, success] = readScratchpad();
        auto noCpp17 = readScratchpad();
        auto scratchpad = std::get<0>(noCpp17);
        auto success = std::get<1>(noCpp17);
        if (!success) {
            return ERROR_TEMPERATURE;
        }

        esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "rawTemperature\n");
        int16_t rawTemperature = (scratchpad.temperature[1] << 8) | scratchpad.temperature[0];
        printf("Temperature is %d\n", rawTemperature);
        return 0.0f;
    }
};

void test(void *pvParameter) {
    esp_log_level_set("*", ESP_LOG_INFO);
    gpio_pullup_en(gpio_num_t(GPIO_DS18B20_0));

    owb_rmt_driver_info rmt_driver_info;
    OneWireBus *bus = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0, RMT_CHANNEL_1, RMT_CHANNEL_0);

    esp_log_write(esp_log_level_t::ESP_LOG_WARN, "ds18b20", "making sensor");
    auto sensor = DS18B20(*bus, DS18B20::Resolution::Bits12);

    while (true) {
        sensor.readTemperature();
    }

    owb_uninitialize(bus);

    vTaskDelay(5000 / portTICK_RATE_MS);
    esp_restart();
}

extern "C" void app_main() {
    nvs_flash_init();
    // xTaskCreate(&mainTask, "mainTask", 2048, NULL, 5, NULL);
    xTaskCreate(&test, "test", 2048, NULL, 5, NULL);
}
