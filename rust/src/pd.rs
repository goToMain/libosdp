//! OSDP specification defines end-point devices as PDs. These devices are
//! responsible for controlling various hardware peripherals (such as LEDs,
//! buzzers, Displays, GPIOs, etc.,) and exposing them in a portable manner to
//! the controlling counter-part, the CP.
//!
//! PD receives commands from the CP and also generates events for activity that
//! happens on the PD itself (such as card read, key press, etc.,) snd sends it
//! to the CP.

use crate::types::{PdCapability, PdInfo};
#[cfg(feature = "std")]
use crate::{
    file::{impl_osdp_file_ops_for, OsdpFile, OsdpFileOps},
    OsdpCommand, OsdpError, OsdpEvent,
};
use alloc::vec::Vec;
use core::ffi::c_void;
use log::{debug, error, info, warn};

type Result<T> = core::result::Result<T, OsdpError>;
type CommandCallback =
    unsafe extern "C" fn(data: *mut c_void, event: *mut libosdp_sys::osdp_cmd) -> i32;

unsafe extern "C" fn log_handler(
    log_level: ::core::ffi::c_int,
    _file: *const ::core::ffi::c_char,
    _line: ::core::ffi::c_ulong,
    msg: *const ::core::ffi::c_char,
) {
    let msg = crate::cstr_to_string(msg);
    let msg = msg.trim();
    match log_level as u32 {
        libosdp_sys::osdp_log_level_e_OSDP_LOG_EMERG => error!("PD: {msg}"),
        libosdp_sys::osdp_log_level_e_OSDP_LOG_ALERT => error!("PD: {msg}"),
        libosdp_sys::osdp_log_level_e_OSDP_LOG_CRIT => error!("PD: {msg}"),
        libosdp_sys::osdp_log_level_e_OSDP_LOG_ERROR => error!("PD: {msg}"),
        libosdp_sys::osdp_log_level_e_OSDP_LOG_WARNING => warn!("PD: {msg}"),
        libosdp_sys::osdp_log_level_e_OSDP_LOG_NOTICE => warn!("PD: {msg}"),
        libosdp_sys::osdp_log_level_e_OSDP_LOG_INFO => info!("PD: {msg}"),
        libosdp_sys::osdp_log_level_e_OSDP_LOG_DEBUG => debug!("PD: {msg}"),
        _ => panic!("Unknown log level"),
    };
}

extern "C" fn trampoline<F>(data: *mut c_void, cmd: *mut libosdp_sys::osdp_cmd) -> i32
where
    F: FnMut(OsdpCommand) -> i32,
{
    let cmd: OsdpCommand = unsafe { (*cmd).into() };
    let callback: &mut F = unsafe { &mut *(data as *mut F) };
    callback(cmd)
}

fn get_trampoline<F>(_closure: &F) -> CommandCallback
where
    F: FnMut(OsdpCommand) -> i32,
{
    trampoline::<F>
}

fn pd_setup(info: PdInfo) -> Result<*mut c_void> {
    let info = info.as_struct();
    let ctx = unsafe { libosdp_sys::osdp_pd_setup(&info) };
    if ctx.is_null() {
        Err(OsdpError::Setup)
    } else {
        Ok(ctx)
    }
}

/// OSDP Peripheral Device (PD) context
#[derive(Debug)]
pub struct PeripheralDevice {
    ctx: *mut libosdp_sys::osdp_t,
}

unsafe impl Send for PeripheralDevice {}

impl PeripheralDevice {
    /// Create a new Peripheral panel object for the list of PDs described by the
    /// corresponding PdInfo list.
    ///
    /// # Example
    ///
    /// ```no_run
    /// use libosdp::{
    ///     PdInfo, PdId, PdCapability, PdCapEntity, OsdpFlag,
    ///     channel::{OsdpChannel, UnixChannel}, ControlPanel,
    /// };
    ///
    /// let stream = UnixChannel::new("conn-1").unwrap();
    /// let pd_info = vec![
    ///     PdInfo::for_pd(
    ///         "PD 101", 101, 115200,
    ///         OsdpFlag::EnforceSecure,
    ///         PdId::from_number(101),
    ///         vec![
    ///             PdCapability::CommunicationSecurity(PdCapEntity::new(1, 1)),
    ///         ],
    ///         OsdpChannel::new::<UnixChannel>(Box::new(stream)),
    ///         [
    ///             0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    ///             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    ///         ]
    ///     ),
    /// ];
    /// let mut cp = ControlPanel::new(pd_info).unwrap();
    /// ```
    pub fn new(info: PdInfo) -> Result<Self> {
        unsafe { libosdp_sys::osdp_set_log_callback(Some(log_handler)) };
        Ok(Self {
            ctx: pd_setup(info)?,
        })
    }

    /// This method is used to periodically refresh the underlying LibOSDP state
    /// and must be called from the application. To meet the OSDP timing
    /// guarantees, this function must be called at least once every 50ms. This
    /// method does not block and returns early if there is nothing to be done.
    pub fn refresh(&mut self) {
        unsafe { libosdp_sys::osdp_pd_refresh(self.ctx) }
    }

    /// Set a vector of [`PdCapability`] for this PD.
    pub fn set_capabilities(&self, cap: &Vec<PdCapability>) {
        let cap: Vec<libosdp_sys::osdp_pd_cap> = cap
            .iter()
            .map(|c| -> libosdp_sys::osdp_pd_cap { c.clone().into() })
            .collect();
        unsafe { libosdp_sys::osdp_pd_set_capabilities(self.ctx, cap.as_ptr()) }
    }

    /// Flush or drop any events queued in this PD (but not delivered to CP yet)
    pub fn flush_events(&mut self) {
        let _ = unsafe { libosdp_sys::osdp_pd_flush_events(self.ctx) };
    }

    /// Queue and a [`OsdpEvent`] for this PD. This will be delivered to CP in
    /// the next POLL.
    pub fn notify_event(&mut self, event: OsdpEvent) -> Result<()> {
        let rc = unsafe { libosdp_sys::osdp_pd_notify_event(self.ctx, &event.into()) };
        if rc < 0 {
            Err(OsdpError::Event)
        } else {
            Ok(())
        }
    }

    /// Set a closure that gets called when this PD receives a command from the
    /// CP.
    pub fn set_command_callback<F>(&mut self, closure: F)
    where
        F: FnMut(OsdpCommand) -> i32,
    {
        unsafe {
            let callback = get_trampoline(&closure);
            libosdp_sys::osdp_pd_set_command_callback(
                self.ctx,
                Some(callback),
                Box::into_raw(Box::new(closure)).cast(),
            )
        }
    }

    /// Check online status of a PD identified by the offset number (in PdInfo
    /// vector in [`PeripheralDevice::new`]).
    pub fn is_online(&self) -> bool {
        let mut buf: u8 = 0;
        unsafe { libosdp_sys::osdp_get_status_mask(self.ctx, &mut buf as *mut u8) };
        buf != 0
    }

    /// Check secure channel status of a PD identified by the offset number
    /// (in PdInfo vector in [`PeripheralDevice::new`]).
    pub fn is_sc_active(&self) -> bool {
        let mut buf: u8 = 0;
        unsafe { libosdp_sys::osdp_get_sc_status_mask(self.ctx, &mut buf as *mut u8) };
        buf != 0
    }
}

impl Drop for PeripheralDevice {
    fn drop(&mut self) {
        unsafe { libosdp_sys::osdp_pd_teardown(self.ctx) }
    }
}

#[cfg(feature = "std")]
impl_osdp_file_ops_for!(PeripheralDevice);
