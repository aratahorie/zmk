/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>

/**
 * @brief Sensitivity scale configuration for cursor movement
 */
struct zmk_pointing_sensitivity_scale {
    uint32_t numerator;
    uint32_t denominator;
};

/**
 * @brief Set trackball cursor sensitivity at runtime
 *
 * @param scale Sensitivity scale (numerator/denominator)
 * @return 0 on success, negative errno on failure
 */
int zmk_pointing_set_cursor_sensitivity(const struct zmk_pointing_sensitivity_scale *scale);

/**
 * @brief Get current trackball cursor sensitivity
 *
 * @param scale Pointer to store current sensitivity scale
 * @return 0 on success, negative errno on failure
 */
int zmk_pointing_get_cursor_sensitivity(struct zmk_pointing_sensitivity_scale *scale);

/**
 * @brief Set scroll sensitivity at runtime
 *
 * @param scale Sensitivity scale (numerator/denominator)
 * @return 0 on success, negative errno on failure
 */
int zmk_pointing_set_scroll_sensitivity(const struct zmk_pointing_sensitivity_scale *scale);

/**
 * @brief Get current scroll sensitivity
 *
 * @param scale Pointer to store current sensitivity scale
 * @return 0 on success, negative errno on failure
 */
int zmk_pointing_get_scroll_sensitivity(struct zmk_pointing_sensitivity_scale *scale);

/**
 * @brief Save current pointing settings to persistent storage
 *
 * @return 0 on success, negative errno on failure
 */
int zmk_pointing_save_settings(void);

/**
 * @brief Load pointing settings from persistent storage
 *
 * @return 0 on success, negative errno on failure
 */
int zmk_pointing_load_settings(void);

/**
 * @brief Reset pointing settings to defaults
 *
 * @return 0 on success, negative errno on failure
 */
int zmk_pointing_reset_settings(void);
