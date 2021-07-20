idf_component_register(SRCS "sntp_config.c" "http_functions.c" "wifi_functions.c" "i2c_config.c" "spi_config.c" "i2c_ds3231.c" "i2c_mma8451q.c" "timer_conf.c" "main.c" "spi_adxl355.c" "spi_mcp356x.c" "sd_config.c"
                    INCLUDE_DIRS "."
                    # Embed the server root certificate into the final binary
                    EMBED_TXTFILES ${project_dir}/server_certs/watchbird.pem)

