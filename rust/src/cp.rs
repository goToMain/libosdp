//! The CP is responsible to connecting to and managing multiple PDs. It is able
//! to send commands to and receive events from PDs.

use crate::{
    commands::OsdpCommand,
    events::OsdpEvent,
    file::{OsdpFile, OsdpFileOps, impl_osdp_file_ops_for},
    osdp_sys,
    OsdpError,
    PdInfo,
    OsdpFlag,
    PdCapability,
    PdId,
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

fn get_trampoline<F>(_closure: &F) -> EventCallback
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

/// OSDP CP device context.
#[derive(Debug)]
pub struct ControlPanel {
    ctx: *mut std::ffi::c_void,
}

impl ControlPanel {
    /// Create a new control panel object for the list of PDs described by the
    /// corresponding PdInfo list.
    ///
    /// # Example
    ///
    /// ```
    /// let pd_info = vec![
    ///     PdInfo::for_cp(
    ///         "PD 101", 101,
    ///         115200,
    ///         OsdpFlag::EnforceSecure,
    ///         OsdpChannel::new::<UnixChannel>(Box::new(stream)),
    ///         [
    ///             0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    ///             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    ///         ]
    ///     ),
    /// ];
    /// let mut cp = ControlPanel::new(pd_info)?;
    /// ```
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

    /// The application must call this method periodically to refresh the
    /// underlying LibOSDP state. To meet the OSDP timing guarantees, this
    /// function must be called at least once every 50ms. This method does not
    /// block and returns early if there is nothing to be done.
    pub fn refresh(&mut self) {
        unsafe { osdp_sys::osdp_cp_refresh(self.ctx) }
    }

    /// Send [`OsdpCommand`] to a PD identified by the offset number (in PdInfo
    /// vector in [`ControlPanel::new`]).
    pub fn send_command(&mut self, pd: i32, cmd: OsdpCommand) -> Result<()> {
        let rc = unsafe { osdp_sys::osdp_cp_send_command(self.ctx, pd, &cmd.into()) };
        if rc < 0 {
            Err(OsdpError::Command)
        } else {
            Ok(())
        }
    }

    /// Set a closure that gets called when a PD sends an event to this CP.
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

    /// Get the [`PdId`] from a PD identified by the offset number (in PdInfo
    /// vector in [`ControlPanel::new`]).
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

    /// Get the [`PdCapability`] from a PD identified by the offset number (in
    /// PdInfo vector in [`ControlPanel::new`]).
    pub fn get_capability(&self, pd: i32, cap: PdCapability) -> Result<PdCapability> {
        let mut cap = cap.into();
        let rc = unsafe { osdp_sys::osdp_cp_get_capability(self.ctx, pd, &mut cap) };
        if rc < 0 {
            Err(OsdpError::Query("capability"))
        } else {
            Ok(cap.into())
        }
    }

    /// Set [`OsdpFlag`] for a PD identified by the offset number (in PdInfo
    /// vector in [`ControlPanel::new`]).
    pub fn set_flag(&mut self, pd: i32, flags: OsdpFlag, value: bool) {
        let rc = unsafe { osdp_sys::osdp_cp_modify_flag(self.ctx, pd, flags.bits(), value) };
        if rc < 0 {
            // OsdpFlag should guarantee that we never fail here. If we did,
            // it's probably best to panic here.
            panic!("osdp_cp_modify_flag failed!")
        }
    }

    /// Check online status of a PD identified by the offset number (in PdInfo
    /// vector in [`ControlPanel::new`]).
    pub fn is_online(&self, pd: i32) -> bool {
        let mut buf: [u8; 16] = [0; 16];
        unsafe { osdp_sys::osdp_get_status_mask(self.ctx, &mut buf as *mut u8) };
        let pos = pd / 8;
        let idx = pd % 8;
        buf[pos as usize] & (1 << idx) != 0
    }

    /// Check secure channel status of a PD identified by the offset number
    /// (in PdInfo vector in [`ControlPanel::new`]).
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

impl_osdp_file_ops_for!(ControlPanel);