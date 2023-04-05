OSDP Secure Channel
===================

OSDP uses AES, with key size of 128 bits (16 bytes). The Secure Channel
(SC) session is initiated by the CP and either the CP or PD can invalidate an
established session (PD by sending a NAK, and CP by starting a new SC
handshake). This document tries to explain the secure channel workflow to enable
application developers to better understand the failure logs emitted by LibOSDP
and it's reasons.

LibOSDP has a working implementation of the secure channel. Any CP/PD that uses
this library will advertise this as an implicit capability. If for some reason
(or for debugging) you don't want to have secure communication, you can call
osdp_pd_set_capabilities() after osdp_setup() to set the compliance level of
``OSDP_PD_CAP_COMMUNICATION_SECURITY`` to 0.

Secure Channel Session
----------------------

A SC session is initiated with a handshake that involves 2 command-reply
transactions between the CP and PD. A session once established can be kept
active endlessly until either party decides to discard it or until a timeout
(400ms) occurs.

The CP starts by sending ``CMD_CHLNG`` for which the PD replies with
``REPLY_CCRYPT``. With this response the CP is able to authenticate the PD and
also compute its session keys. Next, CP sends ``CMD_SCRYPT``, which the PD will
use to authenticate the CP and compute its session keys. If everything has gone
as expected up to this pont, both CP and PD would have a working set of session
keys (s-enc, s-mac1, and s-mac2). The final message of the handshake is the
``REPLY_RMAC_I`` response from the PD. This reply has the initial reply MAC
that the CP will use as the IV for encrypting its next command (MAC chaining).

The above process works in this flow if both the CP and PD have the same SCBK.
If there is a mismatch, the CP will fail to verify the PD's authenticity when it
receives ``REPLY_CCRYPT``. At this point, LibOSDP will try to initiate a new SC
handshake with SCBK-default instead to see if that works (it will if the PD was
set to install-mode). If that too fails, the CP gives up and goes online without
a secure session and retries once every 10 minutes.

Note: This is the default behavior, if you requested ENFORCE_SECURE, things
change slightly (more on that later).

SCBK (Secure Channel Base Key)
------------------------------

This is the primary key that is used to derive the secure channel session keys.
When the application wants to setup a CP or PD, it has to pass the SCBK in
``osdp_pd_info_t::scbk``.

If ``osdp_pd_info_t::scbk`` is set to ``NULL``,
    - In PD mode, the PD is forced into install mode.
    - In CP mode, LibOSDP will look for a master key. If that too doesn't exits
      then the PD is setup with secure channel disabled.

Master Key (MK)
---------------

This a CP specific key that is used to derive the individual PD SCBKs. This can
be passed to osdp_cp_setup(). The MK is common for all PD connected to this CP;
on the other hand, the SCBK has to be unique to each PD.

Latest versions of the OSDP specification discourage the use of master key and
recommend that the CP maintain the SCBKs directly. LibOSDP will print warning
message if master key is used in any of the PDs.

Install Mode
------------

The install-mode is provisioning-time, insecure mode that instructs the PD to
use SCBK-Default (hardcoded key specified by OSDP) and allows the CP to set
a new SCBK. Once the PD receives a new SCBK, it automatically exits the
install-mode so another SCBK key set is not possible.

This mode can be enabled by passing ``OSDP_FLAG_INSTALL_MODE`` in
``osdp_pd_info_t::flags``. This flag should not be passed during normal
operation of the PD.

Entering Install Mode
---------------------

If you are building a PD as a product (in production), you'd have to come up
with some method to force the PD to enter install-mode during provisioning time
to allow the CP to set a new SCBK.

A very trivial method to do this is to have a pin-hole accessible tactile switch
that can be pressed down while the PD reboots (like the WiFi router reset
button). In your application, at boot, if you see this tactile switch pressed
you should pass the ``OSDP_FLAG_INSTALL_MODE`` flag.

Of course, this is not a very secure way of doing this because an attacker who
has physical access to the PD can do the same (see note on Enforce Secure).
There are some other methods that makes it a little difficult for the attacker;
for instance, HID has some special configuration cards that you can present to
the RFID reader soon after boot to force the device into install-mode.

The ENFORCE_SECURE flag
-----------------------

Enforce Secure is a flag that LibOSDP has introduced due to the insecure nature
of the install-mode. A PD/CP can be instructed to operate with some security
consciousness, thereby not allowing some OSDP specified features such as the
install-mode.

The following constraints are enforced when Enforce secure is enabled:
    - Secure Channel cannot be disabled
    - The CP/PD will not operate without a secure channel if setting one up
      failed.
    - Use of SCBK-Default is not allowed (ie., install-mode)
    - Disallow master key based SCBK derivation

When you see a "... due to ENFORCE_SECURE" failure log message, the CP/PD is
doing something that LibOSDP deemed to be a protected action that needs to be
performed under supervision.

Persistence of Keys
-------------------

LibOSDP is not responsible for retaining the SCBK during power cycle of the CP
or PD. It expects the application do this and give it the right SCBK/MK during
osdp_cp/pd_setup().

In case of CP mode, it is the owner of the SCBK/MK (meaning the key originates
here) so the CP app is already aware of the key. In PD mode, when the PD gets a
new SCBK from the CP, the PD app gets a copy of this in the ``OSDP_CMD_KEYSET``
call back.
