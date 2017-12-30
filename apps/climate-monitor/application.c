#include <application.h>

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000) // 1 hour
#define UPDATE_INTERVAL (1 * 1000) // 1 second
#define PUB_NO_CHANGE_INTERVAL (1 * 60 * 1000) // 1 minute

#define TEMPERATURE_TAG_PUB_VALUE_CHANGE 0.2f
#define HUMIDITY_TAG_PUB_VALUE_CHANGE 5.0f
#define LUX_METER_TAG_PUB_VALUE_CHANGE 25.0f
#define BAROMETER_TAG_PUB_VALUE_CHANGE 20.0f

struct {
    event_param_t temperature;
    event_param_t humidity;
    event_param_t illuminance;
    event_param_t pressure;
} params;

// Humidity instance
bc_tag_humidity_t humidity;
event_param_t humidity_event_param = { .next_pub = 0 };

void battery_event_handler(bc_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    float voltage;

    if (bc_module_battery_get_voltage(&voltage))
    {
        bc_radio_pub_battery(&voltage);
    }

    int charge_level;

    if (bc_module_battery_get_charge_level(&charge_level))
    {
        bc_radio_pub_int("battery/-/charge-level", &charge_level);
    }
}

void climate_module_event_handler(bc_module_climate_event_t event, void *event_param)
{
    (void) event_param;

    float value;

    if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_THERMOMETER)
    {
        if (bc_module_climate_get_temperature_celsius(&value))
        {
            if ((fabs(value - params.temperature.value) >= TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (params.temperature.next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_temperature(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value);
                params.temperature.value = value;
                params.temperature.next_pub = bc_scheduler_get_spin_tick() + PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
    else if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_HYGROMETER)
    {
        if (bc_module_climate_get_humidity_percentage(&value))
        {
            if ((fabs(value - params.humidity.value) >= HUMIDITY_TAG_PUB_VALUE_CHANGE) || (params.humidity.next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_humidity(BC_RADIO_PUB_CHANNEL_R3_I2C0_ADDRESS_DEFAULT, &value);
                params.humidity.value = value;
                params.humidity.next_pub = bc_scheduler_get_spin_tick() + PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
    else if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_LUX_METER)
    {
        if (bc_module_climate_get_illuminance_lux(&value))
        {
            if (value < 1)
            {
                value = 0;
            }
            if ((fabs(value - params.illuminance.value) >= LUX_METER_TAG_PUB_VALUE_CHANGE) || (params.illuminance.next_pub < bc_scheduler_get_spin_tick()) ||
                    ((value == 0) && (params.illuminance.value != 0)) || ((value > 1) && (params.illuminance.value == 0)))
            {
                bc_radio_pub_luminosity(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value);
                params.illuminance.value = value;
                params.illuminance.next_pub = bc_scheduler_get_spin_tick() + PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
    else if (event == BC_MODULE_CLIMATE_EVENT_UPDATE_BAROMETER)
    {
        if (bc_module_climate_get_pressure_pascal(&value))
        {
            if ((fabs(value - params.pressure.value) >= BAROMETER_TAG_PUB_VALUE_CHANGE) || (params.pressure.next_pub < bc_scheduler_get_spin_tick()))
            {
                float meter;

                if (!bc_module_climate_get_altitude_meter(&meter))
                {
                    return;
                }

                bc_radio_pub_barometer(BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &value, &meter);
                params.pressure.value = value;
                params.pressure.next_pub = bc_scheduler_get_spin_tick() + PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
}

void humidity_tag_event_handler(bc_tag_humidity_t *self, bc_tag_humidity_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event == BC_TAG_HUMIDITY_EVENT_UPDATE)
    {
        if (bc_tag_humidity_get_humidity_percentage(self, &value))
        {
            if ((fabs(value - param->value) >= HUMIDITY_TAG_PUB_VALUE_CHANGE) || (param->next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_humidity(param->channel, &value);
                param->value = value;
                param->next_pub = bc_scheduler_get_spin_tick() + PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
}

void application_init(void)
{
    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);

    // Initialize battery
#ifndef USE_BATTERY_FORMAT_STANDARD
    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_MINI);
#endif
#ifdef USE_BATTERY_FORMAT_STANDARD
    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_STANDARD);
#endif
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    // Initialize climate module
    bc_module_climate_init();
    bc_module_climate_set_update_interval_all_sensors(UPDATE_INTERVAL);
    bc_module_climate_set_event_handler(climate_module_event_handler, NULL);
    bc_module_climate_measure_all_sensors();

#ifdef USE_HUMIDITY_TAG
    // Initialize humidity tag
    humidity_event_param.channel = BC_RADIO_PUB_CHANNEL_R2_I2C1_ADDRESS_DEFAULT;
    bc_tag_humidity_init(&humidity, BC_TAG_HUMIDITY_REVISION_R2, BC_I2C_I2C1, BC_TAG_HUMIDITY_I2C_ADDRESS_DEFAULT);
    bc_tag_humidity_set_update_interval(&humidity, UPDATE_INTERVAL);
    bc_tag_humidity_set_event_handler(&humidity, humidity_tag_event_handler, &humidity_event_param);
#endif

#ifdef USE_HUMIDITY_TAG
    bc_radio_pairing_request("climate-monitor-tags", VERSION);
#endif
#ifndef USE_HUMIDITY_TAG
    bc_radio_pairing_request("climate-monitor-kit", VERSION);
#endif
}
