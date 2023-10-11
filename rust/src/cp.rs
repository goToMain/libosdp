use crate::{
    common::{PdInfo, PdId},
    commands::OsdpCommand,
};

type Result<T> = anyhow::Result<T, anyhow::Error>;

pub struct ControlPanel {
    ctx: *mut std::ffi::c_void,
}

impl ControlPanel {
    pub fn new(pd_info: &mut Vec<PdInfo>) -> Result<Self> {
        let mut info: Vec<crate::osdp_pd_info_t> = pd_info.iter_mut().map(|i| -> crate::osdp_pd_info_t { i.as_struct() }).collect();
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

    // pub fn set_event_callback(&mut self, callback: fn(OsdpEvent) -> i32) {
    // }

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

    // osdp_cp_get_capability
    // osdp_cp_modify_flag
}

impl Drop for ControlPanel {
    fn drop(&mut self) {
        unsafe { crate::osdp_cp_teardown(self.ctx) }
    }
}
