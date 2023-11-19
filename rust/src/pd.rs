use crate::{
    commands::OsdpCommand,
    common::{PdCapability, PdInfo},
    events::OsdpEvent,
    file::OsdpFile,
    osdp_sys,
};
use log::{debug, error, info, warn};
use std::ffi::c_void;

type Result<T> = anyhow::Result<T, anyhow::Error>;
type CommandCallback =
    unsafe extern "C" fn(data: *mut c_void, event: *mut osdp_sys::osdp_cmd) -> i32;

unsafe extern "C" fn log_handler(
    log_level: ::std::os::raw::c_int,
    _file: *const ::std::os::raw::c_char,
    _line: ::std::os::raw::c_ulong,
    msg: *const ::std::os::raw::c_char,
) {
    let msg = crate::common::cstr_to_string(msg);
    let msg = msg.trim();
    match log_level as u32 {
        osdp_sys::osdp_log_level_e_OSDP_LOG_EMERG => error!("{msg}"),
        osdp_sys::osdp_log_level_e_OSDP_LOG_ALERT => error!("{msg}"),
        osdp_sys::osdp_log_level_e_OSDP_LOG_CRIT => error!("{msg}"),
        osdp_sys::osdp_log_level_e_OSDP_LOG_ERROR => error!("{msg}"),
        osdp_sys::osdp_log_level_e_OSDP_LOG_WARNING => warn!("{msg}"),
        osdp_sys::osdp_log_level_e_OSDP_LOG_NOTICE => warn!("{msg}"),
        osdp_sys::osdp_log_level_e_OSDP_LOG_INFO => info!("{msg}"),
        osdp_sys::osdp_log_level_e_OSDP_LOG_DEBUG => debug!("{msg}"),
        _ => panic!("Unknown log level"),
    };
}

unsafe extern "C" fn trampoline<F>(data: *mut c_void, cmd: *mut osdp_sys::osdp_cmd) -> i32
where
    F: Fn(OsdpCommand) -> i32,
{
    let cmd: OsdpCommand = (*cmd).into();
    let callback = &mut *(data as *mut F);
    callback(cmd)
}

pub fn get_trampoline<F>(_closure: &F) -> CommandCallback
where
    F: Fn(OsdpCommand) -> i32,
{
    trampoline::<F>
}

#[derive(Debug)]
pub struct PeripheralDevice {
    ctx: *mut osdp_sys::osdp_t,
}

impl PeripheralDevice {
    pub fn new(info: &mut PdInfo) -> Result<Self> {
        let mut info = info.as_struct();
        unsafe { osdp_sys::osdp_set_log_callback(Some(log_handler)) };
        let ctx: *mut osdp_sys::osdp_t = unsafe { osdp_sys::osdp_pd_setup(&mut info) };
        if ctx.is_null() {
            anyhow::bail!("Failed to setup PD")
        }
        Ok(Self { ctx })
    }

    pub fn refresh(&mut self) {
        unsafe { osdp_sys::osdp_pd_refresh(self.ctx) }
    }

    pub fn set_capabilities(&self, cap: &mut Vec<PdCapability>) {
        let mut cap: Vec<osdp_sys::osdp_pd_cap> = cap
            .iter_mut()
            .map(|c| -> osdp_sys::osdp_pd_cap { c.as_struct() })
            .collect();
        unsafe { osdp_sys::osdp_pd_set_capabilities(self.ctx, cap.as_mut_ptr()) }
    }

    pub fn flush_events(&mut self) {
        let _ = unsafe { osdp_sys::osdp_pd_flush_events(self.ctx) };
    }

    pub fn notify_event(&mut self, event: OsdpEvent) -> Result<()> {
        let mut event = event.as_struct();
        let rc = unsafe { osdp_sys::osdp_pd_notify_event(self.ctx, &mut event) };
        if rc < 0 {
            anyhow::bail!("Event notify failed!");
        }
        Ok(())
    }

    pub fn set_command_callback(&mut self, mut closure: impl Fn(OsdpCommand) -> i32) {
        let callback = get_trampoline(&closure);
        unsafe {
            osdp_sys::osdp_pd_set_command_callback(
                self.ctx,
                Some(callback),
                &mut closure as *mut _ as *mut c_void,
            )
        }
    }

    pub fn register_file(&self, fm: &mut OsdpFile) -> Result<()> {
        let mut ops = fm.get_ops_struct();
        let rc = unsafe {
            osdp_sys::osdp_file_register_ops(self.ctx, 0, &mut ops as *mut osdp_sys::osdp_file_ops)
        };
        if rc < 0 {
            anyhow::bail!("Failed to register file")
        }
        Ok(())
    }

    pub fn get_file_transfer_status(&mut self) -> Result<(i32, i32)> {
        let mut size: i32 = 0;
        let mut offset: i32 = 0;
        let rc = unsafe {
            osdp_sys::osdp_get_file_tx_status(
                self.ctx,
                0,
                &mut size as *mut i32,
                &mut offset as *mut i32,
            )
        };
        if rc < 0 {
            anyhow::bail!("No file transfer in progress")
        }
        Ok((size, offset))
    }

    pub fn get_version(&self) -> String {
        let s = unsafe { osdp_sys::osdp_get_version() };
        crate::common::cstr_to_string(s)
    }

    pub fn get_source_info(&self) -> String {
        let s = unsafe { osdp_sys::osdp_get_source_info() };
        crate::common::cstr_to_string(s)
    }

    pub fn is_online(&mut self) -> bool {
        let mut buf: u8 = 0;
        unsafe { osdp_sys::osdp_get_status_mask(self.ctx, &mut buf as *mut u8) };
        buf != 0
    }

    pub fn is_sc_active(&mut self) -> bool {
        let mut buf: u8 = 0;
        unsafe { osdp_sys::osdp_get_sc_status_mask(self.ctx, &mut buf as *mut u8) };
        buf != 0
    }
}

impl Drop for PeripheralDevice {
    fn drop(&mut self) {
        unsafe { osdp_sys::osdp_pd_teardown(self.ctx) }
    }
}
