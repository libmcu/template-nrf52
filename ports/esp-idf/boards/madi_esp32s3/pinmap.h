/*
 * SPDX-FileCopyrightText: 2023 권경환 Kyunghwan Kwon <k@libmcu.org>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PINMAP_MADI_ESP32S3_H
#define PINMAP_MADI_ESP32S3_H

#if defined(__cplusplus)
extern "C" {
#endif

#define PINMAP_LED                     35

struct lm_gpio;
struct pinmap_periph {
	struct lm_gpio *led;
};

#if defined(__cplusplus)
}
#endif

#endif /* PINMAP_MADI_ESP32S3_H */
