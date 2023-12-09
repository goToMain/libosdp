//! OSDP specification defines end-point devices as PDs. These devices are
//! responsible for controlling various hardware peripherals (such as LEDs,
//! buzzers, Displays, GPIOs, etc.,) and exposing them in a portable manner to
//! the controlling counter-part, the CP.
//!
//! PD receives commands from the CP and also generates events for activity that
//! happens on the PD itself (such as card read, key press, etc.,) snd sends it
//! to the CP.

use crate::{
    commands::OsdpCommand,
    PdInfo,
    events::OsdpEvent,
    file::{OsdpFile, OsdpFileOps, impl_osdp_file_ops_for},
    osdp_sys,
    OsdpError,
    PdCapability,
};
use log::{debug, error, info, warn};
use std::ffi::c_void;

type Result<T> = std::result::Result<T, OsdpError>;
type CommandCallback =
    unsafe extern "C" fn(data: *mut c_void, event: *mut osdp_sys::osdp_cmd) -> i32;

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

unsafe extern "C" fn trampoline<F>(data: *mut c_void, cmd: *mut osdp_sys::osdp_cmd) -> i32
where
    F: Fn(OsdpCommand) -> i32,
{
    let cmd: OsdpCommand = (*cmd).into();
    let callback = &mut *(data as *mut F);
    callback(cmd)
}

fn get_trampoline<F>(_closure: &F) -> CommandCallback
where
    F: Fn(OsdpCommand) -> i32,
{
    trampoline::<F>
}

fn pd_setup(info: PdInfo) -> Result<*mut c_void> {
    let info = info.as_struct();
    let ctx = unsafe {
        osdp_sys::osdp_pd_setup(&info)
    };
    if ctx.is_null() {
        Err(OsdpError::Setup)
    } else {
        Ok(ctx)
    }
}

/// OSDP Peripheral Device (PD) context
#[derive(Debug)]
pub struct PeripheralDevice {
    ctx: *mut osdp_sys::osdp_t,
}

impl PeripheralDevice {
    /// Create a new Peripheral panel object for the list of PDs described by the
    /// corresponding PdInfo list.
    ///
    /// # Example
    ///
    /// ```
    /// use libosdp::{
    ///     PdInfo, PdId, PdCapability, PdCapEntity, OsdpFlag,
    ///     channel::{OsdpChannel, UnixChannel},
    ///     pd::PeripheralDevice,
    /// };
    ///
    /// let stream = UnixChannel::new("conn-1");
    /// let pd_info = vec![
    ///     PdInfo::for_pd(
    ///         "PD 101", 101,
    ///         115200,
    ///         PdId::from_number(101),
    ///         vec![
    ///             PdCapability::CommunicationSecurity(PdCapEntity::new(1, 1)),
    ///         ],
    ///         OsdpFlag::EnforceSecure,
    ///         OsdpChannel::new::<UnixChannel>(Box::new(stream)),
    ///         [
    ///             0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    ///             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    ///         ]
    ///     ),
    /// ];
    /// let mut cp = PeripheralDevice::new(pd_info)?;
    /// ```
    pub fn new(info: PdInfo) -> Result<Self> {
        unsafe { osdp_sys::osdp_set_log_callback(Some(log_handler)) };
        Ok(Self { ctx: pd_setup(info)? })
    }

    /// This method is used to periodically refresh the underlying LibOSDP state
    /// and must be called from the application. To meet the OSDP timing
    /// guarantees, this function must be called at least once every 50ms. This
    /// method does not block and returns early if there is nothing to be done.
    pub fn refresh(&mut self) {
        unsafe { osdp_sys::osdp_pd_refresh(self.ctx) }
    }

    /// Set a vector of [`PdCapability`] for this PD.
    pub fn set_capabilities(&self, cap: &Vec<PdCapability>) {
        let cap: Vec<osdp_sys::osdp_pd_cap> = cap.iter()
            .map(|c| -> osdp_sys::osdp_pd_cap { c.clone().into() })
            .collect();
        unsafe { osdp_sys::osdp_pd_set_capabilities(self.ctx, cap.as_ptr()) }
    }

    /// Flush or drop any events queued in this PD (but not delivered to CP yet)
    pub fn flush_events(&mut self) {
        let _ = unsafe { osdp_sys::osdp_pd_flush_events(self.ctx) };
    }

    /// Queue and a [`OsdpEvent`] for this PD. This will be delivered to CP in
    /// the next POLL.
    pub fn notify_event(&mut self, event: OsdpEvent) -> Result<()> {
        let rc = unsafe { osdp_sys::osdp_pd_notify_event(self.ctx, &event.into()) };
        if rc < 0 {
            Err(OsdpError::Event)
        } else {
            Ok(())
        }
    }

    /// Set a closure that gets called when this PD receives a command from the
    /// CP.
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

    /// Check online status of a PD identified by the offset number (in PdInfo
    /// vector in [`PeripheralDevice::new`]).
    pub fn is_online(&self) -> bool {
        let mut buf: u8 = 0;
        unsafe { osdp_sys::osdp_get_status_mask(self.ctx, &mut buf as *mut u8) };
        buf != 0
    }

    /// Check secure channel status of a PD identified by the offset number
    /// (in PdInfo vector in [`PeripheralDevice::new`]).
    pub fn is_sc_active(&self) -> bool {
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

impl_osdp_file_ops_for!(PeripheralDevice);