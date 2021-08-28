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

	void logger_init(int log_level, osdp_log_fn_t log_fn)
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

	void get_status_mask(uint8_t *bitmask)
	{
		osdp_get_status_mask(_ctx, bitmask);
	}

	void get_sc_status_mask(uint8_t *bitmask)
	{
		osdp_get_sc_status_mask(_ctx, bitmask);
	}

	void set_command_complete_callback(osdp_command_complete_callback_t cb)
	{
		osdp_set_command_complete_callback(_ctx, cb);
	}

	int file_register_ops(int pd, struct osdp_file_ops *ops)
	{
		return osdp_file_register_ops(_ctx, pd, ops);
	}

	int file_tx_get_status(int pd, int *size, int *offset)
	{
		return osdp_get_file_tx_status(_ctx, pd, size, offset);
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

	void set_event_callback(cp_event_callback_t cb, void *arg)
	{
		osdp_cp_set_event_callback(_ctx, cb, arg);
	}

	int get_pd_id(int pd, struct osdp_pd_id *id)
	{
		return osdp_cp_get_pd_id(_ctx, pd, id);
	}

	int get_capability(int pd, struct osdp_pd_cap *cap)
	{
		return osdp_cp_get_capability(_ctx, pd, cap);
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
