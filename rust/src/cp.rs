use crate::{
    commands::OsdpCommand,
    events::OsdpEvent,
    file::OsdpFile,
    osdp_sys,
    error::OsdpError,
    pdinfo::{PdInfo, OsdpFlag},
    pdcap::PdCapability,
    pdid::PdId,
};
use log::{debug, error, info, warn};
use std::ffi::c_void;

type Result<T> = std::result::Result<T, OsdpError>;
type EventCallback =
    unsafe extern "C" fn(data: *mut c_void, pd: i32, event: *mut osdp_sys::osdp_event) -> i32;

unsafe extern "C" fn log_handler(
    log_level: ::std::os::raw::c_int,
    _file: *const ::std::os::raw::c_char,
    _line: ::std::os::raw::c_ulong,
    msg: *const ::std::os::raw::c_char,
) {
    let msg = crate::cstr_to_string(msg);
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

unsafe extern "C" fn trampoline<F>(
    data: *mut c_void,
    pd: i32,
    event: *mut osdp_sys::osdp_event,
) -> i32
where
    F: Fn(i32, OsdpEvent) -> i32,
{
    let event: OsdpEvent = (*event).into();
    let callback = &mut *(data as *mut F);
    callback(pd, event)
}

pub fn get_trampoline<F>(_closure: &F) -> EventCallback
where
    F: Fn(i32, OsdpEvent) -> i32,
{
    trampoline::<F>
}

fn cp_setup(info: Vec<osdp_sys::osdp_pd_info_t>) -> Result<*mut c_void> {
    let ctx = unsafe {
        osdp_sys::osdp_cp_setup(info.len() as i32, info.as_ptr())
    };
    if ctx.is_null() {
        Err(OsdpError::Setup)
    } else {
        Ok(ctx)
    }
}

#[derive(Debug)]
pub struct ControlPanel {
    ctx: *mut std::ffi::c_void,
}

impl ControlPanel {
    pub fn new(pd_info: Vec<PdInfo>) -> Result<Self> {
        if pd_info.len() > 126 {
            return Err(OsdpError::PdInfo("max PD count exceeded"))
        }
        let info: Vec<osdp_sys::osdp_pd_info_t> = pd_info
            .iter()
            .map(|i| { i.as_struct() })
            .collect();
        unsafe { osdp_sys::osdp_set_log_callback(Some(log_handler)) };
        Ok(Self { ctx: cp_setup(info)? })
    }

    pub fn refresh(&mut self) {
        unsafe { osdp_sys::osdp_cp_refresh(self.ctx) }
    }

    pub fn send_command(&mut self, pd: i32, cmd: OsdpCommand) -> Result<()> {
        let rc = unsafe { osdp_sys::osdp_cp_send_command(self.ctx, pd, &cmd.into()) };
        if rc < 0 {
            Err(OsdpError::Command)
        } else {
            Ok(())
        }
    }

    pub fn set_event_callback(&mut self, mut closure: impl Fn(i32, OsdpEvent) -> i32) {
        let callback = get_trampoline(&closure);
        unsafe {
            osdp_sys::osdp_cp_set_event_callback(
                self.ctx,
                Some(callback),
                &mut closure as *mut _ as *mut c_void,
            );
        }
    }

    pub fn get_pd_id(&self, pd: i32) -> Result<PdId> {
        let mut pd_id: osdp_sys::osdp_pd_id =
            unsafe { std::mem::MaybeUninit::zeroed().assume_init() };
        let rc = unsafe { osdp_sys::osdp_cp_get_pd_id(self.ctx, pd, &mut pd_id) };
        if rc < 0 {
            Err(OsdpError::Query("PdId"))
        } else {
            Ok(pd_id.into())
        }
    }

    pub fn get_capability(&self, pd: i32, cap: PdCapability) -> Result<PdCapability> {
        let mut cap = cap.into();
        let rc = unsafe { osdp_sys::osdp_cp_get_capability(self.ctx, pd, &mut cap) };
        if rc < 0 {
            Err(OsdpError::Query("capability"))
        } else {
            Ok(cap.into())
        }
    }

    pub fn set_flag(&mut self, pd: i32, flags: OsdpFlag, value: bool) {
        let rc = unsafe { osdp_sys::osdp_cp_modify_flag(self.ctx, pd, flags.bits(), value) };
        if rc < 0 {
            // OsdpFlag should guarantee that we never fail here. If we did,
            // it's probably best to panic here.
            panic!("osdp_cp_modify_flag failed!")
        }
    }

    pub fn register_file(&mut self, pd: i32, fm: &mut OsdpFile) -> Result<()> {
        let mut ops = fm.get_ops_struct();
        let rc = unsafe {
            osdp_sys::osdp_file_register_ops(self.ctx, pd, &mut ops as *mut osdp_sys::osdp_file_ops)
        };
        if rc < 0 {
            Err(OsdpError::FileTransfer("ops register"))
        } else {
            Ok(())
        }
    }

    pub fn get_file_transfer_status(&self, pd: i32) -> Result<(i32, i32)> {
        let mut size: i32 = 0;
        let mut offset: i32 = 0;
        let rc = unsafe {
            osdp_sys::osdp_get_file_tx_status(
                self.ctx,
                pd,
                &mut size as *mut i32,
                &mut offset as *mut i32,
            )
        };
        if rc < 0 {
            Err(OsdpError::FileTransfer("transfer status query"))
        } else {
            Ok((size, offset))
        }
    }

    pub fn get_version(&self) -> String {
        let s = unsafe { osdp_sys::osdp_get_version() };
        crate::cstr_to_string(s)
    }

    pub fn get_source_info(&self) -> String {
        let s = unsafe { osdp_sys::osdp_get_source_info() };
        crate::cstr_to_string(s)
    }

    pub fn is_online(&self, pd: i32) -> bool {
        let mut buf: [u8; 16] = [0; 16];
        unsafe { osdp_sys::osdp_get_status_mask(self.ctx, &mut buf as *mut u8) };
        let pos = pd / 8;
        let idx = pd % 8;
        buf[pos as usize] & (1 << idx) != 0
    }

    pub fn is_sc_active(&self, pd: i32) -> bool {
        let mut buf: [u8; 16] = [0; 16];
        unsafe { osdp_sys::osdp_get_sc_status_mask(self.ctx, &mut buf as *mut u8) };
        let pos = pd / 8;
        let idx = pd % 8;
        buf[pos as usize] & (1 << idx) != 0
    }
}

impl Drop for ControlPanel {
    fn drop(&mut self) {
        unsafe { osdp_sys::osdp_cp_teardown(self.ctx) }
    }
}
