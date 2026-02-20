/*
 * SPDX-FileCopyrightText: 2026 Kyunghwan Kwon <k@libmcu.org>
 *
 * SPDX-License-Identifier: MIT
 */

#include "logging.h"

#include <zephyr/sys/printk.h>

static size_t logging_stdout_writer(const void *data, size_t size)
{
	(void)size;

	static char buf[LOGGING_MESSAGE_MAXLEN];
	size_t len = logging_stringify(buf, sizeof(buf) - 1, data);

	buf[len++] = '\n';
	buf[len]   = '\0';

	printk("%s", buf);

	return len;
}

void logging_stdout_backend_init(void)
{
	static struct logging_backend log_console = {
		.write = logging_stdout_writer,
	};

	logging_add_backend(&log_console);
}
