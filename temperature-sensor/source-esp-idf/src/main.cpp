#include <array>
#include <iostream>

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

void test(void *pvParameter) {
    esp_log_level_set("*", ESP_LOG_INFO);
    gpio_pullup_en(gpio_num_t(GPIO_DS18B20_0));

    owb_rmt_driver_info rmt_driver_info;
    OneWireBus *bus = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(bus, true);

    DS18B20_Info device;
    ds18b20_init_solo(&device, bus);

    ds18b20_use_crc(&device, true);  // enable CRC check for temperature readings
    // ds18b20_set_resolution(&device, DS18B20_RESOLUTION::DS18B20_RESOLUTION_10_BIT);

    while (true) {
        bool is_present = false;
        owb_reset(bus, &is_present);
        owb_write_byte(bus, OWB_ROM_SKIP);
        owb_write_byte(bus, 0x44);

        vTaskDelay(200 / portTICK_RATE_MS);

        float temperature = 0.0f;
        ds18b20_read_temp(&device, &temperature);

        printf("Temperature readings (degrees C): %f\n", temperature);
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