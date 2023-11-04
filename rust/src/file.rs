use crate::libosdp;
use std::{
    path::PathBuf,
    fs::File,
    ffi::c_void,
    os::unix::prelude::FileExt
};

pub struct OsdpFile {
    id: i32,
    path: PathBuf,
    file: Option<File>,
    size: usize,
}

unsafe extern "C"
fn raw_file_open(data: *mut c_void, file_id: i32, size: *mut i32) -> i32 {
    let ctx = &mut *(data as *mut OsdpFile);
    if ctx.file.is_some() || file_id != ctx.id {
        return -1;
    }
    let file = match File::open(ctx.path.as_os_str()) {
        Ok(f) => f,
        Err(_) => return -1,
    };
    ctx.size = file.metadata().unwrap().len() as usize;
    ctx.file = Some(file);
    unsafe {
        *size = ctx.size as i32;
    }
    return 0;
}

unsafe extern "C"
fn raw_file_read(data: *mut c_void, buf: *mut c_void, size: i32, offset: i32) -> i32 {
    let ctx = &mut *(data as *mut OsdpFile);
    if ctx.file.is_none() {
        return -1;
    }
    let file = ctx.file.as_ref().unwrap();
    let mut read_buf = vec![0u8; size as usize];
    let len = match file.read_at(&mut read_buf, offset as u64) {
        Ok(len) => len as i32,
        Err(_) => -1,
    };
    std::ptr::copy_nonoverlapping(read_buf.as_mut_ptr(), buf as *mut u8, len as usize);
    return len;
}

unsafe extern "C"
fn raw_file_write(data: *mut c_void, buf: *const c_void, size: i32, offset: i32) -> i32 {
    let ctx = &mut *(data as *mut OsdpFile);
    if ctx.file.is_none() {
        return -1;
    }
    let mut write_buf = vec![0u8; size as usize];
    std::ptr::copy_nonoverlapping(buf as *mut u8, write_buf.as_mut_ptr(), size as usize);
    let file = ctx.file.as_ref().unwrap();
    match file.write_at(&write_buf, offset as u64) {
        Ok(len) => len as i32,
        Err(_) => -1,
    }
}

unsafe extern "C"
fn raw_file_close(data: *mut c_void) -> i32 {
    let ctx = &mut *(data as *mut OsdpFile);
    if ctx.file.is_none() {
        return -1;
    }
    let _ = ctx.file.take().unwrap();
    return 0;
}

impl OsdpFile {
    pub fn new(id: i32, path: PathBuf) -> Self {
        Self {
            id,
            path,
            file: None,
            size: 0,
        }
    }

    pub fn get_ops_struct(&mut self) -> libosdp::osdp_file_ops {
        libosdp::osdp_file_ops {
            arg: self as *mut _ as *mut c_void,
            open: Some(raw_file_open),
            read: Some(raw_file_read),
            write: Some(raw_file_write),
            close: Some(raw_file_close),
        }
    }
}
