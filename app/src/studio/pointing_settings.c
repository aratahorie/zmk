/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/settings/settings.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

#include <zmk/studio/pointing.h>
#include <drivers/input_processor.h>

#define POINTING_SETTINGS_PREFIX "pointing"
#define CURSOR_SCALE_KEY "cursor_scale"
#define SCROLL_SCALE_KEY "scroll_scale"

// Current runtime settings
static struct zmk_pointing_sensitivity_scale cursor_scale = {
    .numerator = 1,
    .denominator = 1,
};

static struct zmk_pointing_sensitivity_scale scroll_scale = {
    .numerator = 1,
    .denominator = 1,
};

static bool settings_loaded = false;

// Find the cursor scaler device (zip_xy_scaler)
static const struct device *get_cursor_scaler_device(void) {
    // Get the zip_xy_scaler device by label
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(zip_xy_scaler));
    LOG_INF("Cursor scaler device: %p, name: %s", dev, dev ? dev->name : "NULL");
    if (!device_is_ready(dev)) {
        LOG_ERR("Cursor scaler device (zip_xy_scaler) not ready");
        return NULL;
    }
    LOG_INF("Cursor scaler device is ready");
    return dev;
}

// Find the scroll scaler device (zip_scroll_scaler)
static const struct device *get_scroll_scaler_device(void) {
    // Get the zip_scroll_scaler device by label
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(zip_scroll_scaler));
    if (!device_is_ready(dev)) {
        LOG_ERR("Scroll scaler device (zip_scroll_scaler) not ready");
        return NULL;
    }
    return dev;
}

int zmk_pointing_set_cursor_sensitivity(const struct zmk_pointing_sensitivity_scale *scale) {
    LOG_INF("zmk_pointing_set_cursor_sensitivity called: %d/%d", scale ? scale->numerator : 0, scale ? scale->denominator : 0);

    if (!scale || scale->denominator == 0) {
        LOG_ERR("Invalid scale parameters");
        return -EINVAL;
    }

    const struct device *scaler_dev = get_cursor_scaler_device();
    if (!scaler_dev) {
        LOG_ERR("Failed to get cursor scaler device");
        return -ENODEV;
    }

    LOG_INF("About to set override on device %p: %d/%d", scaler_dev, scale->numerator, scale->denominator);
    int ret = zmk_input_processor_scaler_set_override(scaler_dev, scale->numerator, scale->denominator);
    if (ret < 0) {
        LOG_ERR("Failed to set cursor sensitivity override: %d", ret);
        return ret;
    }

    cursor_scale = *scale;
    LOG_INF("Cursor sensitivity successfully set to %d/%d", scale->numerator, scale->denominator);
    return 0;
}

int zmk_pointing_get_cursor_sensitivity(struct zmk_pointing_sensitivity_scale *scale) {
    if (!scale) {
        return -EINVAL;
    }

    *scale = cursor_scale;
    return 0;
}

int zmk_pointing_set_scroll_sensitivity(const struct zmk_pointing_sensitivity_scale *scale) {
    if (!scale || scale->denominator == 0) {
        return -EINVAL;
    }

    const struct device *scaler_dev = get_scroll_scaler_device();
    if (!scaler_dev) {
        // If scroll scaler is not available, just store the setting
        LOG_WRN("Scroll scaler device not available, storing setting only");
        scroll_scale = *scale;
        return 0;
    }

    int ret = zmk_input_processor_scaler_set_override(scaler_dev, scale->numerator, scale->denominator);
    if (ret < 0) {
        LOG_ERR("Failed to set scroll sensitivity: %d", ret);
        return ret;
    }

    scroll_scale = *scale;
    LOG_INF("Scroll sensitivity set to %d/%d", scale->numerator, scale->denominator);
    return 0;
}

int zmk_pointing_get_scroll_sensitivity(struct zmk_pointing_sensitivity_scale *scale) {
    if (!scale) {
        return -EINVAL;
    }

    *scale = scroll_scale;
    return 0;
}

// Settings callbacks
static int pointing_settings_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg) {
    const char *next;

    if (settings_name_steq(name, CURSOR_SCALE_KEY, &next) && !next) {
        if (len != sizeof(struct zmk_pointing_sensitivity_scale)) {
            return -EINVAL;
        }

        int rc = read_cb(cb_arg, &cursor_scale, sizeof(cursor_scale));
        if (rc >= 0) {
            LOG_INF("Loaded cursor scale: %d/%d", cursor_scale.numerator, cursor_scale.denominator);
            return 0;
        }
        return rc;
    }

    if (settings_name_steq(name, SCROLL_SCALE_KEY, &next) && !next) {
        if (len != sizeof(struct zmk_pointing_sensitivity_scale)) {
            return -EINVAL;
        }

        int rc = read_cb(cb_arg, &scroll_scale, sizeof(scroll_scale));
        if (rc >= 0) {
            LOG_INF("Loaded scroll scale: %d/%d", scroll_scale.numerator, scroll_scale.denominator);
            return 0;
        }
        return rc;
    }

    return -ENOENT;
}

static int pointing_settings_commit(void) {
    settings_loaded = true;

    // Apply loaded cursor sensitivity
    const struct device *cursor_scaler_dev = get_cursor_scaler_device();
    if (cursor_scaler_dev) {
        int ret = zmk_input_processor_scaler_set_override(cursor_scaler_dev, cursor_scale.numerator, cursor_scale.denominator);
        if (ret < 0) {
            LOG_WRN("Failed to apply loaded cursor sensitivity: %d", ret);
        } else {
            LOG_INF("Applied cursor sensitivity: %d/%d", cursor_scale.numerator, cursor_scale.denominator);
        }
    }

    // Apply loaded scroll sensitivity
    const struct device *scroll_scaler_dev = get_scroll_scaler_device();
    if (scroll_scaler_dev) {
        int ret = zmk_input_processor_scaler_set_override(scroll_scaler_dev, scroll_scale.numerator, scroll_scale.denominator);
        if (ret < 0) {
            LOG_WRN("Failed to apply loaded scroll sensitivity: %d", ret);
        } else {
            LOG_INF("Applied scroll sensitivity: %d/%d", scroll_scale.numerator, scroll_scale.denominator);
        }
    }

    return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(pointing, POINTING_SETTINGS_PREFIX, NULL, pointing_settings_set, pointing_settings_commit, NULL);

int zmk_pointing_save_settings(void) {
    int ret;

    ret = settings_save_one(POINTING_SETTINGS_PREFIX "/" CURSOR_SCALE_KEY, &cursor_scale, sizeof(cursor_scale));
    if (ret < 0) {
        LOG_ERR("Failed to save cursor scale: %d", ret);
        return ret;
    }

    ret = settings_save_one(POINTING_SETTINGS_PREFIX "/" SCROLL_SCALE_KEY, &scroll_scale, sizeof(scroll_scale));
    if (ret < 0) {
        LOG_ERR("Failed to save scroll scale: %d", ret);
        return ret;
    }

    LOG_INF("Pointing settings saved");
    return 0;
}

int zmk_pointing_load_settings(void) {
    if (!settings_loaded) {
        LOG_WRN("Settings not yet loaded by subsystem");
        return -EAGAIN;
    }

    return 0;
}

int zmk_pointing_reset_settings(void) {
    int ret;

    // Reset to defaults
    cursor_scale.numerator = 1;
    cursor_scale.denominator = 1;
    scroll_scale.numerator = 1;
    scroll_scale.denominator = 1;

    // Delete from storage
    ret = settings_delete(POINTING_SETTINGS_PREFIX "/" CURSOR_SCALE_KEY);
    if (ret < 0 && ret != -ENOENT) {
        LOG_ERR("Failed to delete cursor scale: %d", ret);
        return ret;
    }

    ret = settings_delete(POINTING_SETTINGS_PREFIX "/" SCROLL_SCALE_KEY);
    if (ret < 0 && ret != -ENOENT) {
        LOG_ERR("Failed to delete scroll scale: %d", ret);
        return ret;
    }

    // Clear runtime overrides
    const struct device *cursor_scaler_dev = get_cursor_scaler_device();
    if (cursor_scaler_dev) {
        zmk_input_processor_scaler_clear_override(cursor_scaler_dev);
    }

    const struct device *scroll_scaler_dev = get_scroll_scaler_device();
    if (scroll_scaler_dev) {
        zmk_input_processor_scaler_clear_override(scroll_scaler_dev);
    }

    LOG_INF("Pointing settings reset to defaults");
    return 0;
}
