# OSDP Secure Channel

OSDP uses AES-128 to establish a Secure Channel (SC) between the CP and PD. A
SC is one over which mutually authenticated and encrypted data is sent. The
algorithm detailed below does a MAC-chaining that allows reply-attach rejection.

Once a SC is established, it is closed in any of the below conditions:

- unable to verify MAC
- Unable to decrypt a message
- time-out

## OSDP Secure Channel handshake overview:

The secure channel is set-up by following a sequence of command exchanges
between the CP and the PD. All of these steps must be performed in order,
atomically for the SC to be initialized.

If `SCS_11` to `SCS_14` ia successful, then a secure channel is established.
This is called a secure channel session. All keys generated are ephemeral and
must not be stored.

The `R-MAC_I` from `SCS_14` is used as IV for next command. The PD uses `CMAC`
from the the command it received as IV for it message. This Chaining of MAC is
required to maintain the secure channel. If the synchronization is lost, then
the current session is discarded and a new session is restarted.

All communication over a secure channel will happen with either SCS_15 (with
authentication; without encryption) or SCS_17 (with authentication and
encryption). SCS_15 is used in development mode or for commands that do not have
a data portion.

## OSDP Secure Channel handshake

### SCS_11 -- CP to PD

In CMD_CHLNG, the CP send its random number `CP_RND[8]` to the PD.

### SCS_12 -- PD to CP

On receiving `CP_RND[8]` the PD does the following:

- Generates it's random `PD_RAND[8]`
- Computes `SCBK`
- Computes session keys (`S-ENC`, `S-MAC1`, `S-MAC2`)
- Computes `PD_CRYPT` (as below)

```
PD_CRYPT = AES-ECB ( CP_RND[8] || PD_RND[8], S-ENC )
```

PD responds with REPLY_CCRYPT, sending its `CLIENT_UID` (client unique
identifier), random number and `PD_CRYPT`.

### SCS_13 -- CP to PD

On receiving `PD_RND[8]`, `CLIENT_UID`, and `PD_CRYPT`, from the PD, the CP the
following:

- Computes `SCBK`
- Computes session keys (`S-ENC`, `S-MAC1`, `S-MAC2`)
- Computes `PD_CRYPT` and verifies it against received `PD_CRYPT` to
  authenticate PD
- Computes `CP_CRYPT` (as below)

```
CP_CRYPT = AES-ECB ( pd_random[8] || cp_random[8], S-ENC )
```

In CMD_SCRYPT, CP sends `CP_CRYPT` to the PD.

### SCS_14 -- PD to CP

On receiving `CP_CRYPT`,

- Computes `CP_CRYPT` and verifies it against received `CP_CRYPT` to
  authenticate CP
- Generates `R-MAC_I` (initial reply MAC)

In REPLY_RMAC_I, PD sends the `R-MAC_I` to CP. This is expected to be used as
IV for the next message the CP send to the PD.

## Key derivation

### Secure Channel Base Key:

`CLIENT_UID` is obtained from the PD in response to command CMD_PDID.

```
SCBK = AES-ECB ( CLIENT_UID || (~CLIENT_UID), MASTER_KEY )
```

### Session Keys:

```
S-ENC  = AES-ECB ( { 0x01, 0x82, CP_RND[0 to 5], 0, 0, ... }, SCBK )
S-MAC1 = AES-ECB ( { 0x01, 0x01, CP_RND[0 to 5], 0, 0, ... }, SCBK )
S-MAC2 = AES-ECB ( { 0x01, 0x02, CP_RND[0 to 5], 0, 0, ... }, SCBK )
```

## Message Authentication Code (MAC)

MAC is computed for and appended only to messages whose SEC_BLK_TYPE is SCS_15,
SCS_16, SCS_17, and SCS_18. The AES algorithm is applied in CBC mode using
S-MAC1 as the key for all blocks, except the last one, and using S-MAC2 as the
key for the last block. If the message contains only one block, then only
S-MAC2 is used.

MAC for data blocks B[1] .. B[N] (post padding) is computed as:

```
IV1 = LAST_RECEIVED_MAC
AES-CBC ( IV1, B[1] to B[N-1], SMAC-1 )
IV2 = B[N-1]
MAC = AES-ECB ( IV2, B[N], SMAC-2 )
```
