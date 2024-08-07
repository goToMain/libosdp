Command and Reply Codes
=======================

The following commands and replies are reserved by the protocol for various
actions. The following tables show their descriptions and whether or not this
implementation of OSDP supports those commands.

Commands
--------

+-----------------+---------+-------------------------------------------------------+-----------+
| Command         | Value   | Description                                           | Support   |
+=================+=========+=======================================================+===========+
| CMD_POLL        | 0x60    | Poll                                                  | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_ID          | 0x61    | ID Report Request                                     | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_CAP         | 0x62    | Peripheral Device Capabilities Request                | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_LSTAT       | 0x64    | Local Status Report Request                           | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_ISTAT       | 0x65    | Input Status Report Request                           | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_OSTAT       | 0x66    | Output Status Report Request                          | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_RSTAT       | 0x67    | Reader Status Tamper Report Request                   | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_OUT         | 0x68    | Output Control Command                                | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_LED         | 0x69    | Reader LED Control Command                            | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_BUZ         | 0x6A    | Reader Buzzer Control Command                         | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_TEXT        | 0x6B    | Reader Text Output Command                            | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_COMSET      | 0x6E    | Communication Configuration Command                   | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_BIOREAD     | 0x73    | Scan and send biometric data                          | No        |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_BIOMATCH    | 0x74    | Scan and match biometric data                         | No        |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_KEYSET      | 0x75    | Encryption Key Set                                    | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_CHLNG       | 0x76    | Challenge and Secure Session Initialization Request   | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_SCRYPT      | 0x77    | Server's Random Number and Server Cryptogram          | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_ACURXSIZE   | 0x7B    | Maximum Acceptable Reply Size                         | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_FILETRANSFER| 0x7C    | File transfer command                                 | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_ABORT       | 0xA2    | Stop Multi Part Message                               | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_MFG         | 0x80    | Manufacturer Specific Command                         | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_XWR         | 0xA1    | Extended write data                                   | No        |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_ABORT       | 0xA2    | Abort PD operation                                    | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_PIVDATA     | 0xA3    | Get PIV Data                                          | No        |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_GENAUTH     | 0xA4    | Request Authenticate                                  | No        |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_CRAUTH      | 0xA5    | Request Crypto Response                               | No        |
+-----------------+---------+-------------------------------------------------------+-----------+
| CMD_KEEPACTIVE  | 0xA7    | Keep secure channel active                            | Yes       |
+-----------------+---------+-------------------------------------------------------+-----------+

Responses
---------

+--------------------+---------+----------------------------------------------------------+-----------+
| Response           | Value   | Description                                              | Support   |
+====================+=========+==========================================================+===========+
| REPLY_ACK          | 0x40    | General Acknowledge, Nothing to Report                   | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_NAK          | 0x41    | Negative Acknowledge – SIO Comm Handler Error Response   | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_PDID         | 0x45    | Device Identification Report                             | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_PDCAP        | 0x46    | Device Capabilities Report                               | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_LSTATR       | 0x48    | Local Status Report                                      | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_IASTR        | 0x49    | Input Status Report                                      | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_OSTATR       | 0x4A    | Output Status Report                                     | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_RSTATR       | 0x4B    | Reader Status Tamper Report                              | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_RAW          | 0x50    | Card Data Report, Raw Bit Array                          | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_FMT          | 0x51    | Card Data Report, Character Array                        | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_KEYPAD       | 0x53    | Keypad Data Report                                       | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_COM          | 0x54    | Communication Configuration Report                       | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_BIOREADR     | 0x57    | Biometric Data                                           | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_BIOMATCHR    | 0x58    | Biometric Match Result                                   | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_CCRYPT       | 0x76    | Client's ID and Client's Random Number                   | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_BUSY         | 0x79    | PD Is Busy Reply                                         | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_RMAC_I       | 0x78    | Client Cryptogram Packet and the Initial R-MAC           | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_FTSTAT       | 0x7A    | File transfer status                                     | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_PIVDATAR     | 0x80    | PIV data reply                                           | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_GENAUTHR     | 0x81    | Authentication response                                  | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_CRAUTHR      | 0x82    | Response to challenge                                    | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_MFGSTATR     | 0x83    | Manufacturer specific status                             | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_MFGERRR      | 0x84    | Manufacturer specific error                              | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_MFGREP       | 0x90    | Manufacturer specific reply                              | Yes       |
+--------------------+---------+----------------------------------------------------------+-----------+
| REPLY_XRD          | 0xB1    | Extended read response                                   | No        |
+--------------------+---------+----------------------------------------------------------+-----------+
