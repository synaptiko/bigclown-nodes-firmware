#include <application.h>

#define BATTERY_UPDATE_INTERVAL (60 * 60 * 1000) // 1 hour

#define UPDATE_INTERVAL (1 * 1000) // 1 second
#define PUB_NO_CHANGE_INTERVAL (1 * 60 * 1000) // 1 minute

#define TEMPERATURE_TAG_PUB_VALUE_CHANGE 0.2f

// Temperature instance
bc_tag_temperature_t temperature;
event_param_t temperature_event_param = { .next_pub = 0 };

bc_module_pir_t pir;
uint16_t pir_event_count = 0;

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

void temperature_tag_event_handler(bc_tag_temperature_t *self, bc_tag_temperature_event_t event, void *event_param)
{
    float value;
    event_param_t *param = (event_param_t *)event_param;

    if (event == BC_TAG_TEMPERATURE_EVENT_UPDATE)
    {
        if (bc_tag_temperature_get_temperature_celsius(self, &value))
        {
            if ((fabs(value - param->value) >= TEMPERATURE_TAG_PUB_VALUE_CHANGE) || (param->next_pub < bc_scheduler_get_spin_tick()))
            {
                bc_radio_pub_temperature(param->channel, &value);
                param->value = value;
                param->next_pub = bc_scheduler_get_spin_tick() + PUB_NO_CHANGE_INTERVAL;
            }
        }
    }
}

void pir_event_handler(bc_module_pir_t *self, bc_module_pir_event_t event, void *event_param)
{
    (void) self;
    (void) event_param;

    if (event == BC_MODULE_PIR_EVENT_MOTION)
    {
        pir_event_count++;

        bc_radio_pub_event_count(BC_RADIO_PUB_EVENT_PIR_MOTION, &pir_event_count);
    }
}

void application_init(void)
{
    bc_radio_init(BC_RADIO_MODE_NODE_SLEEPING);

    // Initialize battery
    bc_module_battery_init(BC_MODULE_BATTERY_FORMAT_MINI);
    bc_module_battery_set_event_handler(battery_event_handler, NULL);
    bc_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    // Initialize temperature
    temperature_event_param.channel = BC_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_ALTERNATE;
    bc_tag_temperature_init(&temperature, BC_I2C_I2C0, BC_TAG_TEMPERATURE_I2C_ADDRESS_ALTERNATE);
    bc_tag_temperature_set_update_interval(&temperature, UPDATE_INTERVAL);
    bc_tag_temperature_set_event_handler(&temperature, temperature_tag_event_handler, &temperature_event_param);

    // Initialize pir module
    bc_module_pir_init(&pir);
    bc_module_pir_set_sensitivity(&pir, BC_MODULE_PIR_SENSITIVITY_VERY_HIGH);
    bc_module_pir_set_event_handler(&pir, pir_event_handler, NULL);

    bc_radio_pairing_request("motion-detector", VERSION);
}
