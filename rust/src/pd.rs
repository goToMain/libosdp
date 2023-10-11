use std::ffi::c_void;
use crate::{
    common::{PdInfo, PdCapability},
    events::OsdpEvent,
    commands::OsdpCommand
};
type Result<T> = anyhow::Result<T, anyhow::Error>;

type CommandCallback = unsafe extern "C" fn (data: *mut c_void, event: *mut crate::osdp_cmd) -> i32;

unsafe extern "C" fn trampoline<F>(data: *mut c_void, cmd: *mut crate::osdp_cmd) -> i32
where
    F: Fn(OsdpCommand) -> i32,
{
    let cmd: OsdpCommand  = (*cmd).into();
    let callback = &mut *(data as *mut F);
    callback(cmd)
}

pub fn get_trampoline<F>(_closure: &F) -> CommandCallback
where
    F: Fn(OsdpCommand) -> i32,
{
    trampoline::<F>
}

pub struct PeripheralDevice {
    ctx: *mut crate::osdp_t,
}

impl PeripheralDevice {
    pub fn new(info: &mut PdInfo) -> Result<Self> {
        let mut info = info.as_struct();
        let ctx: *mut crate::osdp_t =  unsafe { crate::osdp_pd_setup(&mut info) };
        if ctx.is_null() {
            anyhow::bail!("Failed to setup PD")
        }
        Ok(Self {
            ctx,
        })
    }

    pub fn refresh(&mut self) {
        unsafe { crate::osdp_pd_refresh(self.ctx) }
    }

    pub fn set_capabilities(&self, cap: &mut Vec<PdCapability>) {
        let mut cap: Vec<crate::osdp_pd_cap> = cap.iter_mut().map(|c| -> crate::osdp_pd_cap { c.as_struct() }).collect();
        unsafe { crate::osdp_pd_set_capabilities(self.ctx, cap.as_mut_ptr()) }
    }

    pub fn flush_events(&mut self) {
        let _ = unsafe { crate::osdp_pd_flush_events(self.ctx) };
    }

    pub fn notify_event(&mut self, event: OsdpEvent) -> Result<()> {
        let mut event = event.as_struct();
        let rc = unsafe {
            crate::osdp_pd_notify_event(self.ctx, &mut event)
        };
        if rc < 0 {
            anyhow::bail!("Event notify failed!");
        }
        Ok(())
    }

    pub fn set_command_callback(&mut self, mut closure: fn(OsdpCommand) -> i32) {
        let callback = get_trampoline(&closure);
        unsafe {
            crate::osdp_pd_set_command_callback(
                self.ctx,
                Some(callback),
                &mut closure as *mut _ as *mut c_void
            )
        }
    }
}

impl Drop for PeripheralDevice {
    fn drop(&mut self) {
        unsafe { crate::osdp_pd_teardown(self.ctx) }
    }
}
