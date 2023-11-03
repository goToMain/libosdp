use std::ffi::c_void;
use crate::{
    common::{PdInfo, PdId, PdCapability, OsdpFlag},
    commands::OsdpCommand,
    events::OsdpEvent,
    file::OsdpFile,
};

type Result<T> = anyhow::Result<T, anyhow::Error>;
type EventCallback = unsafe extern "C" fn (data: *mut c_void, pd: i32, event: *mut crate::osdp_event) -> i32;

unsafe extern "C"
fn trampoline<F>(data: *mut c_void, pd: i32, event: *mut crate::osdp_event) -> i32
where
    F: Fn(i32, OsdpEvent) -> i32,
{
    let event: OsdpEvent  = (*event).into();
    let callback = &mut *(data as *mut F);
    callback(pd, event)
}

pub fn get_trampoline<F>(_closure: &F) -> EventCallback
where
    F: Fn(i32, OsdpEvent) -> i32,
{
    trampoline::<F>
}

pub struct ControlPanel {
    ctx: *mut std::ffi::c_void,
}

impl ControlPanel {
    pub fn new(pd_info: &mut Vec<PdInfo>) -> Result<Self> {
        if pd_info.len() > 126 {
            anyhow::bail!("Max PD count exceeded")
        }
        let mut info: Vec<crate::osdp_pd_info_t> = pd_info.iter_mut().map(|i| -> crate::osdp_pd_info_t { i.as_struct() }).collect();
        unsafe { crate::osdp_set_log_callback(Some(crate::common::log_handler)) };
        let ctx = unsafe {
            crate::osdp_cp_setup2(pd_info.len() as i32, info.as_mut_ptr())
        };
        if ctx.is_null() {
            anyhow::bail!("CP setup failed!")
        }
        Ok(Self {
            ctx
        })
    }

    pub fn refresh(&mut self) {
        unsafe { crate::osdp_cp_refresh(self.ctx) }
    }

    pub fn send_command(&mut self, pd: i32, cmd: &OsdpCommand) -> Result<()> {
        let mut cmd = cmd.as_struct();
        let rc = unsafe {
            crate::osdp_cp_send_command(self.ctx, pd, std::ptr::addr_of_mut!(cmd))
        };
        if rc < 0 {
            anyhow::bail!("Failed to send command");
        }
        Ok(())
    }

    pub fn set_event_callback(&mut self, mut closure: fn(i32, OsdpEvent) -> i32) {
        let callback = get_trampoline(&closure);
        unsafe {
            crate::osdp_cp_set_event_callback(
                self.ctx,
                Some(callback),
                &mut closure as *mut _ as *mut c_void
            );
        }
    }

    pub fn get_pd_id(&mut self, pd: i32) -> Result<PdId> {
        let mut pd_id: crate::osdp_pd_id = unsafe {
            std::mem::MaybeUninit::zeroed().assume_init()
        };
        let rc = unsafe {
            crate::osdp_cp_get_pd_id(self.ctx, pd, &mut pd_id)
        };
        if rc < 0 {
            anyhow::bail!("Failed to get PD ID");
        }
        Ok(pd_id.into())
    }

    pub fn get_capability(&mut self, pd: i32, cap: PdCapability) -> Result<PdCapability> {
        let mut cap = cap.as_struct();
        let rc = unsafe {
            crate::osdp_cp_get_capability(self.ctx, pd, &mut cap)
        };
        if rc < 0 {
            anyhow::bail!("Failed to read capability")
        }
        Ok(cap.into())
    }

    pub fn set_flag(&mut self, pd: i32, flags: OsdpFlag, value: bool) {
        let rc = unsafe {
            crate::osdp_cp_modify_flag(self.ctx, pd, flags.bits(), value)
        };
        if rc < 0 {
            // OsdpFlag should guarantee that we never fail here. If we did,
            // it's probably best to panic here.
            panic!("osdp_cp_modify_flag failed!")
        }
    }

    pub fn register_file(&mut self, pd: i32, fm: &mut OsdpFile) -> Result<()> {
        let mut ops = fm.get_ops_struct();
        let rc = unsafe {
            crate::osdp_file_register_ops(self.ctx, pd, &mut ops as *mut crate::osdp_file_ops)
        };
        if rc < 0 {
            anyhow::bail!("Failed to register file")
        }
        Ok(())
    }

    pub fn get_file_transfer_status(&mut self, pd: i32) -> Result<(i32, i32)> {
        let mut size: i32 = 0;
        let mut offset: i32 = 0;
        let rc = unsafe {
            crate::osdp_get_file_tx_status(
                self.ctx,
                pd,
                &mut size as *mut i32,
                &mut offset as *mut i32
            )
        };
        if rc < 0 {
            anyhow::bail!("No file transfer in progress")
        }
        Ok((size, offset))
    }

    pub fn get_version(&self) -> String {
        let s = unsafe { crate::osdp_get_version() };
        crate::common::cstr_to_string(s)
    }

    pub fn get_source_info(&self) -> String {
        let s = unsafe { crate::osdp_get_source_info() };
        crate::common::cstr_to_string(s)
    }

    pub fn is_online(&mut self, pd: i32) -> bool {
        let mut buf: [u8; 16] = [0; 16];
        unsafe {
            crate::osdp_get_status_mask(
                self.ctx,
                &mut buf as *mut u8
            )
        };
        let pos = pd / 8;
        let idx = pd % 8;
        buf[pos as usize] & (1 << idx) != 0
    }

    pub fn is_sc_active(&mut self, pd: i32) -> bool {
        let mut buf: [u8; 16] = [0; 16];
        unsafe {
            crate::osdp_get_sc_status_mask(
                self.ctx,
                &mut buf as *mut u8
            )
        };
        let pos = pd / 8;
        let idx = pd % 8;
        buf[pos as usize] & (1 << idx) != 0
    }
}

impl Drop for ControlPanel {
    fn drop(&mut self) {
        unsafe { crate::osdp_cp_teardown(self.ctx) }
    }
}
