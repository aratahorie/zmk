/*
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk_studio, CONFIG_ZMK_STUDIO_LOG_LEVEL);

#include <zmk/studio/pointing.h>
#include <zmk/studio/rpc.h>

// TODO: Once protobuf definitions are added to zmk-studio-messages, uncomment this
// ZMK_RPC_SUBSYSTEM(pointing)

// TODO: This will be defined in the protobuf when ready
// #define POINTING_RESPONSE(type, ...) ZMK_RPC_RESPONSE(pointing, type, __VA_ARGS__)

/*
 * RPC Handler: set_sensitivity
 *
 * TODO: Implement once protobuf schema is ready
 *
 * This handler should:
 * 1. Parse the SetSensitivityRequest from the RPC
 * 2. Validate numerator and denominator (non-zero, reasonable range)
 * 3. Call zmk_pointing_set_cursor_sensitivity() with the cursor scale
 * 4. Optionally call zmk_pointing_set_scroll_sensitivity() if provided
 * 5. Call zmk_pointing_save_settings() to persist
 * 6. Return SetSensitivityResponse with ok=true or error code
 */
#if 0
zmk_studio_Response set_sensitivity(const zmk_studio_Request *req) {
    LOG_DBG("set_sensitivity RPC called");
    const zmk_pointing_SetSensitivityRequest *set_req =
        &req->subsystem.pointing.request_type.set_sensitivity;

    // Validate request
    if (set_req->cursor.denominator == 0) {
        return POINTING_RESPONSE(set_sensitivity, {
            .which_result = zmk_pointing_SetSensitivityResponse_err_tag,
            .result = {.err = zmk_pointing_SetSensitivityErrorCode_SET_SENSITIVITY_ERR_INVALID}
        });
    }

    // Set cursor sensitivity
    struct zmk_pointing_sensitivity_scale cursor = {
        .numerator = set_req->cursor.numerator,
        .denominator = set_req->cursor.denominator,
    };

    int ret = zmk_pointing_set_cursor_sensitivity(&cursor);
    if (ret < 0) {
        LOG_ERR("Failed to set cursor sensitivity: %d", ret);
        return POINTING_RESPONSE(set_sensitivity, {
            .which_result = zmk_pointing_SetSensitivityResponse_err_tag,
            .result = {.err = zmk_pointing_SetSensitivityErrorCode_SET_SENSITIVITY_ERR_UNSUPPORTED}
        });
    }

    // Set scroll sensitivity if provided
    if (set_req->has_scroll && set_req->scroll.denominator != 0) {
        struct zmk_pointing_sensitivity_scale scroll = {
            .numerator = set_req->scroll.numerator,
            .denominator = set_req->scroll.denominator,
        };

        ret = zmk_pointing_set_scroll_sensitivity(&scroll);
        if (ret < 0) {
            LOG_WRN("Failed to set scroll sensitivity: %d", ret);
        }
    }

    // Persist settings
    ret = zmk_pointing_save_settings();
    if (ret < 0) {
        LOG_ERR("Failed to save pointing settings: %d", ret);
        return POINTING_RESPONSE(set_sensitivity, {
            .which_result = zmk_pointing_SetSensitivityResponse_err_tag,
            .result = {.err = zmk_pointing_SetSensitivityErrorCode_SET_SENSITIVITY_ERR_STORAGE}
        });
    }

    return POINTING_RESPONSE(set_sensitivity, {
        .which_result = zmk_pointing_SetSensitivityResponse_ok_tag,
        .result = {.ok = true}
    });
}
#endif

/*
 * RPC Handler: get_sensitivity (optional)
 *
 * TODO: Implement once protobuf schema is ready
 *
 * This handler should:
 * 1. Call zmk_pointing_get_cursor_sensitivity() to get current cursor scale
 * 2. Call zmk_pointing_get_scroll_sensitivity() to get current scroll scale
 * 3. Return GetSensitivityResponse with current values
 */
#if 0
zmk_studio_Response get_sensitivity(const zmk_studio_Request *req) {
    LOG_DBG("get_sensitivity RPC called");

    struct zmk_pointing_sensitivity_scale cursor, scroll;
    zmk_pointing_get_cursor_sensitivity(&cursor);
    zmk_pointing_get_scroll_sensitivity(&scroll);

    zmk_pointing_GetSensitivityResponse resp = zmk_pointing_GetSensitivityResponse_init_zero;
    resp.cursor.numerator = cursor.numerator;
    resp.cursor.denominator = cursor.denominator;
    resp.has_scroll = true;
    resp.scroll.numerator = scroll.numerator;
    resp.scroll.denominator = scroll.denominator;

    return POINTING_RESPONSE(get_sensitivity, resp);
}
#endif

// TODO: Register RPC handlers once protobuf is ready
// ZMK_RPC_SUBSYSTEM_HANDLER(pointing, set_sensitivity, ZMK_STUDIO_RPC_HANDLER_SECURED);
// ZMK_RPC_SUBSYSTEM_HANDLER(pointing, get_sensitivity, ZMK_STUDIO_RPC_HANDLER_UNSECURED);

/*
 * Settings reset callback
 * Called when core.reset_settings RPC is invoked
 */
static int pointing_settings_reset(void) {
    LOG_DBG("Resetting pointing settings");
    return zmk_pointing_reset_settings();
}

// Register settings reset handler
ZMK_RPC_SUBSYSTEM_SETTINGS_RESET(pointing, pointing_settings_reset);
