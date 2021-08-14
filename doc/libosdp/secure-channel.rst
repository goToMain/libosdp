Secure Channel
==============

OSDP uses AES-128, with key size of 128 bits (16 bytes).

LibOSDP has a working implementation of the secure channel. Any CP/PD that uses
this library will advertises this as an implicit capability. If for some reason
(or for debugging) you don't want to have secure communication, you can call
osdp_pd_set_capabilities() after osdp_setup() to set the compliance level of
``OSDP_PD_CAP_COMMUNICATION_SECURITY`` to 0.

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
uses SCBK-Default (hardcoded key specified by OSDP) and allows the CP to set
a new SCBK. Once the PD received a new SCBK, it automatically exits the
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
button). In the your application, at boot, if you see this tactile switch
pressed, you should pass the ``OSDP_FLAG_INSTALL_MODE`` flag.

Of course, this not a very secure way of doing this because an attacker who has
physical access to the PD can do the same (see note on Enforce Secure). There
other some other methods that makes it a little difficult for the attacker. For
instance, HID has some special configuration cards that you can present to the
RFID reader soon after boot to force the device into install-mode.

Enforce Secure
--------------

Enforce Secure is a special mode that LibOSDP has introduced due to the insecure
nature of the install-mode stuffs. A PD/CP can be instructed to operate with the
some security consciousness, thereby not allowing some OSDP specified features
such as the install-mode.

The following enforced when Enforce secure is enabled:
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

LibOSDP is not responsible of retaining the SCBK during power cycle of the CP or
PD. It expects the application do this and give it the right SCBK/MK during
osdp_cp/pd_setup().

In case of CP mode, it is the owner of the SCBK/MK (meaning the key originates
here) so the CP app is already aware of the key. In PD mode, when the PD gets a
new SCBK from the CP, the PD app gets a copy of this in the ``OSDP_CMD_KEYSET``
call back.