/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_input_processor_scaler

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <drivers/input_processor.h>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct scaler_config {
    uint8_t type;
    size_t codes_len;
    uint16_t codes[];
};

struct scaler_data {
    bool override_enabled;
    uint32_t override_mul;
    uint32_t override_div;
};

static int scale_val(struct input_event *event, uint32_t mul, uint32_t div,
                     struct zmk_input_processor_state *state) {
    int16_t value_mul = event->value * (int16_t)mul;

    if (state && state->remainder) {
        value_mul += *state->remainder;
    }

    int16_t scaled = value_mul / (int16_t)div;

    if (state && state->remainder) {
        *state->remainder = value_mul - (scaled * (int16_t)div);
    }

    LOG_DBG("scaled %d with %d/%d to %d with remainder %d", event->value, mul, div, scaled,
            (state && state->remainder) ? *state->remainder : 0);

    event->value = scaled;

    return 0;
}

static int scaler_handle_event(const struct device *dev, struct input_event *event, uint32_t param1,
                               uint32_t param2, struct zmk_input_processor_state *state) {
    const struct scaler_config *cfg = dev->config;
    struct scaler_data *data = dev->data;

    if (event->type != cfg->type) {
        return ZMK_INPUT_PROC_CONTINUE;
    }

    for (int i = 0; i < cfg->codes_len; i++) {
        if (cfg->codes[i] == event->code) {
            // Use override parameters if enabled, otherwise use devicetree params
            uint32_t mul = (data && data->override_enabled) ? data->override_mul : param1;
            uint32_t div = (data && data->override_enabled) ? data->override_div : param2;
            return scale_val(event, mul, div, state);
        }
    }

    return ZMK_INPUT_PROC_CONTINUE;
}

static struct zmk_input_processor_driver_api scaler_driver_api = {
    .handle_event = scaler_handle_event,
};

#define SCALER_INST(n)                                                                             \
    static struct scaler_data scaler_data_##n = {                                                  \
        .override_enabled = false,                                                                 \
        .override_mul = 1,                                                                         \
        .override_div = 1,                                                                         \
    };                                                                                             \
    static const struct scaler_config scaler_config_##n = {                                        \
        .type = DT_INST_PROP_OR(n, type, INPUT_EV_REL),                                            \
        .codes_len = DT_INST_PROP_LEN(n, codes),                                                   \
        .codes = DT_INST_PROP(n, codes),                                                           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &scaler_data_##n, &scaler_config_##n, POST_KERNEL,       \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &scaler_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SCALER_INST)

// Public API for runtime parameter override
int zmk_input_processor_scaler_set_override(const struct device *dev, uint32_t numerator, uint32_t denominator) {
    if (!dev || denominator == 0) {
        return -EINVAL;
    }

    struct scaler_data *data = dev->data;
    if (!data) {
        return -ENODEV;
    }

    data->override_mul = numerator;
    data->override_div = denominator;
    data->override_enabled = true;

    LOG_INF("Scaler override set: %d/%d", numerator, denominator);
    return 0;
}

int zmk_input_processor_scaler_clear_override(const struct device *dev) {
    if (!dev) {
        return -EINVAL;
    }

    struct scaler_data *data = dev->data;
    if (!data) {
        return -ENODEV;
    }

    data->override_enabled = false;

    LOG_INF("Scaler override cleared");
    return 0;
}