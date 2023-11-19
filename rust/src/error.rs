use thiserror::Error;

#[derive(Debug, Default, Error)]
pub enum OsdpError {
    #[error("Invalid PdInfo {0}")]
    PdInfo(&'static str),
    #[error("Invalid OsdpCommand")]
    Command,
    #[error("Invalid OsdpEvent")]
    Event,
    #[error("Failed to query {0} from device")]
    Query(&'static str),
    #[error("File transfer failed: {0}")]
    FileTransfer(&'static str),
    #[error("Failed to setup device")]
    Setup,
    #[error("Type {0} parse error")]
    Parse(String),
    #[error("IO Error")]
    Io(#[from] std::io::Error),
    #[default]
    #[error("Unknown/Unspecified error")]
    Unknown,
}

impl From<std::convert::Infallible> for OsdpError {
    fn from(_: std::convert::Infallible) -> Self {
        unreachable!()
    }
}