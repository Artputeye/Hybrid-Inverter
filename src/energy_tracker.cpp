#include "energy_tracker.h"

// History buckets are ordered oldest to newest; the last slot is the active period.
float gridHourlyEnergy[24] = {0};
float solarHourlyEnergy[24] = {0};
float gridDailyEnergy[30] = {0};
float solarDailyEnergy[30] = {0};
float gridMonthlyEnergy[12] = {0};
float solarMonthlyEnergy[12] = {0};

static const char *jsonFilePath = "/energy_history.json";
static const unsigned long historySaveIntervalMs = 15UL * 60UL * 1000UL;

static bool historyInitialized = false;
static bool historyFileLoaded = false;
static unsigned long lastHistorySaveMillis = 0;
static int64_t lastHourKey = 0;
static int64_t lastDayKey = 0;
static int64_t lastMonthKey = 0;
static float lastGridDailySnapshot = 0.0f;
static float lastSolarDailySnapshot = 0.0f;
static float lastGridMonthlySnapshot = 0.0f;
static float lastSolarMonthlySnapshot = 0.0f;

// Convert a calendar date to a continuous day number for detecting skipped periods.
static int64_t civilDayKey(int year, unsigned int month, unsigned int day)
{
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned int yearOfEra = static_cast<unsigned int>(year - era * 400);
    const unsigned int adjustedMonth = month > 2 ? month - 3 : month + 9;
    const unsigned int dayOfYear = (153 * adjustedMonth + 2) / 5 + day - 1;
    const unsigned int dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 + dayOfYear;
    return static_cast<int64_t>(era) * 146097 + dayOfEra;
}

static bool readCurrentPeriodKeys(int64_t &hourKey, int64_t &dayKey, int64_t &monthKey)
{
    if (rtc.year < 2020 || rtc.month < 1 || rtc.month > 12 || rtc.day < 1 || rtc.day > 31 || rtc.hour < 0 || rtc.hour > 23)
    {
        return false;
    }

    dayKey = civilDayKey(rtc.year, static_cast<unsigned int>(rtc.month), static_cast<unsigned int>(rtc.day));
    hourKey = dayKey * 24 + rtc.hour;
    monthKey = static_cast<int64_t>(rtc.year) * 12 + (rtc.month - 1);
    return true;
}

static float positiveCounter(float value)
{
    return isfinite(value) && value > 0.0f ? value : 0.0f;
}

static float counterDelta(float current, float previous)
{
    current = positiveCounter(current);
    previous = positiveCounter(previous);
    return current >= previous ? current - previous : current;
}

static void clearFloatArray(float *values, size_t size)
{
    for (size_t index = 0; index < size; index++)
    {
        values[index] = 0.0f;
    }
}

static void shiftHistory(float *values, size_t size, int64_t periods)
{
    if (periods <= 0)
    {
        return;
    }

    if (periods >= static_cast<int64_t>(size))
    {
        clearFloatArray(values, size);
        return;
    }

    const size_t shift = static_cast<size_t>(periods);
    for (size_t index = 0; index < size - shift; index++)
    {
        values[index] = values[index + shift];
    }
    for (size_t index = size - shift; index < size; index++)
    {
        values[index] = 0.0f;
    }
}

static void addToCurrentBucket(float *values, size_t size, float delta)
{
    values[size - 1] += delta;
}

static void writeHistoryArray(JsonDocument &doc, const char *key, const float *values, size_t size)
{
    JsonArray array = doc[key].to<JsonArray>();
    for (size_t index = 0; index < size; index++)
    {
        array.add(values[index]);
    }
}

static void readHistoryArray(JsonArrayConst array, float *values, size_t size)
{
    clearFloatArray(values, size);
    const size_t count = array.size() < size ? array.size() : size;
    for (size_t index = 0; index < count; index++)
    {
        float value = array[index] | 0.0f;
        values[index] = positiveCounter(value);
    }
}

static void migrateLegacyArray(JsonArrayConst array, float *values, size_t size)
{
    clearFloatArray(values, size);
    const size_t count = array.size() < size ? array.size() : size;
    for (size_t index = 0; index < count; index++)
    {
        float value = array[count - 1 - index] | 0.0f;
        values[size - count + index] = positiveCounter(value);
    }
}

// Persist all six time ranges plus counter snapshots used to calculate the next delta.
bool saveEnergyToJson()
{
    JsonDocument doc;
    doc["version"] = 2;
    writeHistoryArray(doc, "grid_hourly", gridHourlyEnergy, 24);
    writeHistoryArray(doc, "solar_hourly", solarHourlyEnergy, 24);
    writeHistoryArray(doc, "grid_daily", gridDailyEnergy, 30);
    writeHistoryArray(doc, "solar_daily", solarDailyEnergy, 30);
    writeHistoryArray(doc, "grid_monthly", gridMonthlyEnergy, 12);
    writeHistoryArray(doc, "solar_monthly", solarMonthlyEnergy, 12);

    doc["hour_key"] = lastHourKey;
    doc["day_key"] = lastDayKey;
    doc["month_key"] = lastMonthKey;
    doc["last_grid_daily"] = lastGridDailySnapshot;
    doc["last_solar_daily"] = lastSolarDailySnapshot;
    doc["last_grid_monthly"] = lastGridMonthlySnapshot;
    doc["last_solar_monthly"] = lastSolarMonthlySnapshot;

    File file = LittleFS.open(jsonFilePath, FILE_WRITE);
    if (!file)
    {
        Serial.println(F("[ENERGY HISTORY] Failed to open history file for writing"));
        return false;
    }

    const size_t written = serializeJson(doc, file);
    file.close();
    if (written == 0)
    {
        Serial.println(F("[ENERGY HISTORY] Failed to serialize history"));
        return false;
    }

    return true;
}

// Load the current schema, or migrate the previous grid-only history arrays.
bool loadEnergyFromJson()
{
    if (!LittleFS.exists(jsonFilePath))
    {
        return false;
    }

    File file = LittleFS.open(jsonFilePath, FILE_READ);
    if (!file)
    {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error)
    {
        Serial.printf("[ENERGY HISTORY] Failed to parse history: %s\n", error.c_str());
        return false;
    }

    historyFileLoaded = true;
    if ((doc["version"] | 0) >= 2)
    {
        readHistoryArray(doc["grid_hourly"].as<JsonArrayConst>(), gridHourlyEnergy, 24);
        readHistoryArray(doc["solar_hourly"].as<JsonArrayConst>(), solarHourlyEnergy, 24);
        readHistoryArray(doc["grid_daily"].as<JsonArrayConst>(), gridDailyEnergy, 30);
        readHistoryArray(doc["solar_daily"].as<JsonArrayConst>(), solarDailyEnergy, 30);
        readHistoryArray(doc["grid_monthly"].as<JsonArrayConst>(), gridMonthlyEnergy, 12);
        readHistoryArray(doc["solar_monthly"].as<JsonArrayConst>(), solarMonthlyEnergy, 12);

        lastHourKey = doc["hour_key"] | 0LL;
        lastDayKey = doc["day_key"] | 0LL;
        lastMonthKey = doc["month_key"] | 0LL;
        lastGridDailySnapshot = positiveCounter(doc["last_grid_daily"] | 0.0f);
        lastSolarDailySnapshot = positiveCounter(doc["last_solar_daily"] | 0.0f);
        lastGridMonthlySnapshot = positiveCounter(doc["last_grid_monthly"] | 0.0f);
        lastSolarMonthlySnapshot = positiveCounter(doc["last_solar_monthly"] | 0.0f);
        historyInitialized = lastHourKey != 0 && lastDayKey != 0 && lastMonthKey != 0;
    }
    else
    {
        migrateLegacyArray(doc["hourly"].as<JsonArrayConst>(), gridHourlyEnergy, 24);
        migrateLegacyArray(doc["daily"].as<JsonArrayConst>(), gridDailyEnergy, 30);
    }

    return true;
}

// Update active buckets from counter deltas and save at 15-minute intervals.
void updateEnergyHistory(bool force)
{
    const unsigned long now = millis();
    if (!force && now - lastHistorySaveMillis < historySaveIntervalMs)
    {
        return;
    }

    int64_t hourKey;
    int64_t dayKey;
    int64_t monthKey;
    if (!readCurrentPeriodKeys(hourKey, dayKey, monthKey))
    {
        return;
    }

    const float currentGridDaily = positiveCounter(energy_kWh);
    const float currentSolarDaily = positiveCounter(solar_kWh);
    const float currentGridMonthly = positiveCounter(energy_m_kWh);
    const float currentSolarMonthly = positiveCounter(solar_m_kWh);

    if (!historyInitialized)
    {
        lastHourKey = hourKey;
        lastDayKey = dayKey;
        lastMonthKey = monthKey;
        lastGridDailySnapshot = currentGridDaily;
        lastSolarDailySnapshot = currentSolarDaily;
        lastGridMonthlySnapshot = currentGridMonthly;
        lastSolarMonthlySnapshot = currentSolarMonthly;

        // Seed current day/month totals so the chart has values immediately after startup.
        if (!historyFileLoaded)
        {
            gridDailyEnergy[29] = currentGridDaily;
            solarDailyEnergy[29] = currentSolarDaily;
            gridMonthlyEnergy[11] = currentGridMonthly;
            solarMonthlyEnergy[11] = currentSolarMonthly;
        }

        historyInitialized = true;
        lastHistorySaveMillis = now;
        saveEnergyToJson();
        return;
    }

    const int64_t elapsedHours = hourKey > lastHourKey ? hourKey - lastHourKey : (hourKey < lastHourKey ? 1 : 0);
    const int64_t elapsedDays = dayKey > lastDayKey ? dayKey - lastDayKey : (dayKey < lastDayKey ? 1 : 0);
    const int64_t elapsedMonths = monthKey > lastMonthKey ? monthKey - lastMonthKey : (monthKey < lastMonthKey ? 1 : 0);

    shiftHistory(gridHourlyEnergy, 24, elapsedHours);
    shiftHistory(solarHourlyEnergy, 24, elapsedHours);
    shiftHistory(gridDailyEnergy, 30, elapsedDays);
    shiftHistory(solarDailyEnergy, 30, elapsedDays);
    shiftHistory(gridMonthlyEnergy, 12, elapsedMonths);
    shiftHistory(solarMonthlyEnergy, 12, elapsedMonths);

    const float gridDailyDelta = counterDelta(currentGridDaily, lastGridDailySnapshot);
    const float solarDailyDelta = counterDelta(currentSolarDaily, lastSolarDailySnapshot);
    const float gridMonthlyDelta = counterDelta(currentGridMonthly, lastGridMonthlySnapshot);
    const float solarMonthlyDelta = counterDelta(currentSolarMonthly, lastSolarMonthlySnapshot);

    addToCurrentBucket(gridHourlyEnergy, 24, gridDailyDelta);
    addToCurrentBucket(solarHourlyEnergy, 24, solarDailyDelta);
    addToCurrentBucket(gridDailyEnergy, 30, gridDailyDelta);
    addToCurrentBucket(solarDailyEnergy, 30, solarDailyDelta);
    addToCurrentBucket(gridMonthlyEnergy, 12, gridMonthlyDelta);
    addToCurrentBucket(solarMonthlyEnergy, 12, solarMonthlyDelta);

    lastHourKey = hourKey;
    lastDayKey = dayKey;
    lastMonthKey = monthKey;
    lastGridDailySnapshot = currentGridDaily;
    lastSolarDailySnapshot = currentSolarDaily;
    lastGridMonthlySnapshot = currentGridMonthly;
    lastSolarMonthlySnapshot = currentSolarMonthly;
    lastHistorySaveMillis = now;
    saveEnergyToJson();
}

// Mount LittleFS and restore history after energy counters have loaded.
void initEnergyTracker()
{
    if (!LittleFS.begin(true))
    {
        Serial.println(F("[ENERGY HISTORY] LittleFS mount failed"));
        return;
    }

    loadEnergyFromJson();
    lastHistorySaveMillis = millis();
    updateEnergyHistory(true);
}