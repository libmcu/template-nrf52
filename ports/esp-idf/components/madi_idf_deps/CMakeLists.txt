# SPDX-License-Identifier: MIT

set(madi_idf_required_components
	app_update
	bt
	driver
	efuse
	esp-tls
	esp_eth
	esp_http_client
	esp_http_server
	esp_https_ota
	esp_psram
	esp_timer
	esp_wifi
	espcoredump
	esptool_py
	freertos
	mbedtls
	nvs_flash
	nvs_sec_provider
	pthread
	remote_wifi_host
	spi_flash
	tcp_transport
)

set(madi_idf_version "$ENV{IDF_VERSION}")
if (NOT madi_idf_version AND DEFINED IDF_VERSION_MAJOR)
	set(madi_idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}.${IDF_VERSION_PATCH}")
endif()

if (NOT madi_idf_version OR madi_idf_version VERSION_GREATER_EQUAL "5.0.0")
	list(APPEND madi_idf_required_components esp_adc)
else()
	list(APPEND madi_idf_required_components esp_adc_cal)
endif()

idf_component_register(
	PRIV_REQUIRES
		${madi_idf_required_components}
)
