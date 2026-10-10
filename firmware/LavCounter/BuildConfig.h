#pragma once
// Physical VL53L0X/VL53L1X with automatic identification. Keep existing physical data paths across upgrades.
namespace lav {
constexpr const char* SENSOR_MODE="physical";
constexpr const char* JOURNAL_PATH="/presence-v2.jsonl";
constexpr const char* CONFIG_NAMESPACE="lavcounter";
}
