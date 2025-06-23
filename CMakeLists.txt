idf_component_register(
    SRCS "src/robotka.cpp"
         "src/_librk_context.cpp"
         "src/_librk_motors.cpp"
         "src/_librk_smartleds.cpp"
         "src/_librk_wifi.cpp"
         "src/fmt/format.cc"
    INCLUDE_DIRS "src"
    REQUIRES "rb3204-rbcx-library"
             "esp32-mcp3008-linesensor"
             "smartleds"
             "rb3201-rbprotocol-library"
             "esp32-rbgridui"
             "espressif__arduino-esp32"
             "spi_flash"
             "nvs_flash"
             "wifi_provisioning"
             "esp_wifi"
             "esp_netif"
             "esp_event"
             "esp_driver_gpio"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE -DRK_DISABLE_BLE=1)
