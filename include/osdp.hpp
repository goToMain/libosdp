/*
 * Copyright (c) 2021-2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <osdp.h>

/**
 * @file: LibOSDP classical wrapper. See osdp.h for documentation.
 */

namespace OSDP {

class Common {
public:
	Common() {}

	void logger_init(int log_level, int (*log_fn)(const char *fmt, ...))
	{
		osdp_logger_init(log_level, log_fn);
	}

	const char *get_version()
	{
		return osdp_get_version();
	}

	const char *get_source_info()
	{
		return osdp_get_source_info();
	}

	uint32_t get_status_mask()
	{
		return osdp_get_status_mask(_ctx);
	}

	uint32_t get_sc_status_mask()
	{
		return osdp_get_sc_status_mask(_ctx);
	}

protected:
	osdp_t *_ctx;
};

class ControlPanel : public Common {
public:
	ControlPanel()
	{
	}

	~ControlPanel()
	{
		osdp_cp_teardown(_ctx);
	}

	bool setup(int num_pd, osdp_pd_info_t *info, uint8_t *master_key)
	{
		_ctx = osdp_cp_setup(num_pd, info, master_key);
		return _ctx != nullptr;
	}

	void refresh()
	{
		osdp_cp_refresh(_ctx);
	}

	int send_command(int pd, struct osdp_cmd *cmd)
	{
		return osdp_cp_send_command(_ctx, pd, cmd);
	}
};

class PeripheralDevice : public Common {
public:
	PeripheralDevice()
	{
	}

	~PeripheralDevice()
	{
		osdp_pd_teardown(_ctx);
	}

	bool setup(osdp_pd_info_t *info)
	{
		_ctx = osdp_pd_setup(info);
		return _ctx != nullptr;
	}

	void refresh()
	{
		osdp_pd_refresh(_ctx);
	}

	void set_command_callback(pd_commnand_callback_t cb)
	{
		osdp_pd_set_command_callback(_ctx, cb, _ctx);
	}

	int notify_event(struct osdp_event *event)
	{
		return osdp_pd_notify_event(_ctx, event);
	}
};

}; /* namespace OSDP */
