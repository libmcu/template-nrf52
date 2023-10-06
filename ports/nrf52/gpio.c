/*
 * SPDX-FileCopyrightText: 2023 Kyunghwan Kwon <k@libmcu.org>
 *
 * SPDX-License-Identifier: MIT
 */

#include "libmcu/gpio.h"

#include <errno.h>

#include "nrf_gpio.h"
#include "pinmap.h"

struct gpio {
	struct gpio_api api;

	uint16_t pin;
	gpio_callback_t callback;
	void *callback_ctx;
};

static void enable_output(uint32_t pin_number)
{
	nrf_gpio_cfg_output(pin_number);
}

static int enable_gpio(struct gpio *self)
{
	switch (self->pin) {
	case PINMAP_LED:
		enable_output(self->pin);
		break;
	default:
		return -ERANGE;
	}

	return 0;
}

static int disable_gpio(struct gpio *self)
{
	nrf_gpio_cfg_default(self->pin);
	return 0;
}

static int set_gpio(struct gpio *self, int value)
{
	if (value) {
		nrf_gpio_pin_set(self->pin);
	} else {
		nrf_gpio_pin_clear(self->pin);
	}
	return 0;
}

static int get_gpio(struct gpio *self)
{
	if (nrf_gpio_pin_dir_get(self->pin) == NRF_GPIO_PIN_DIR_OUTPUT) {
		return (int)nrf_gpio_pin_out_read(self->pin);
	}
	return (int)nrf_gpio_pin_read(self->pin);
	//return (int)nrf_gpio_pin_sense_get(self->pin);
}

static int register_callback(struct gpio *self,
		gpio_callback_t callback, void *ctx)
{
	self->callback = callback;
	self->callback_ctx = ctx;
	return 0;
}

struct gpio *gpio_create(uint16_t pin)
{
	static struct gpio led;

	struct gpio *p;

	switch (pin) {
	case PINMAP_LED:
		p = &led;
		break;
	default:
		return NULL;
	}

	p->pin = pin;

	p->api = (struct gpio_api) {
		.enable = enable_gpio,
		.disable = disable_gpio,
		.set = set_gpio,
		.get = get_gpio,
		.register_callback = register_callback,
	};

	return p;
}

void gpio_delete(struct gpio *self)
{
	(void)self;
}
