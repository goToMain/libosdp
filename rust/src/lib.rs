//!
#![warn(missing_debug_implementations)]
#![warn(rust_2018_idioms)]
// #![warn(missing_docs)]

pub mod osdp_sys;
pub mod pdinfo;
pub mod cp;
pub mod pd;
pub mod commands;
pub mod events;
pub mod channel;
pub mod file;
pub mod error;
pub mod pdcap;
pub mod pdid;

pub fn cstr_to_string(s: *const ::std::os::raw::c_char) -> String {
    let s = unsafe {
        std::ffi::CStr::from_ptr(s)
    };
    s.to_str().unwrap().to_owned()
}