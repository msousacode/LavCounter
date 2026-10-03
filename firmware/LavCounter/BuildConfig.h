#pragma once
// Arduino IDE: simulation enabled. Change to 0 to use the physical sensor.
// Upload without erasing flash to preserve configuration and history.
// CLI builds override this value with -DLAVCOUNTER_SENSOR_MOCK=0 or 1.
#ifndef LAVCOUNTER_SENSOR_MOCK
#define LAVCOUNTER_SENSOR_MOCK 1
#endif
#if LAVCOUNTER_SENSOR_MOCK != 0 && LAVCOUNTER_SENSOR_MOCK != 1
#error LAVCOUNTER_SENSOR_MOCK must be 0 or 1
#endif
namespace lav {
constexpr bool SENSOR_MOCK=LAVCOUNTER_SENSOR_MOCK==1;
constexpr const char* SENSOR_MODE=SENSOR_MOCK?"mock":"physical";
constexpr const char* JOURNAL_PATH=SENSOR_MOCK?"/presence-mock-v2.jsonl":"/presence-v2.jsonl";
constexpr const char* CONFIG_NAMESPACE=SENSOR_MOCK?"lavcounterMock":"lavcounter";
}
