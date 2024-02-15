--
--  Copyright (c) 2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
--
--  SPDX-License-Identifier: Apache-2.0

-- Header
local header = ProtoField.new("Header", "OSDP.Header", ftypes.BYTES)
local mark = ProtoField.uint8("OSDP.Mark", "Mark byte", base.HEX)
local som = ProtoField.uint8("OSDP.SOM", "Start of message", base.HEX)
local type = ProtoField.new("Message Type", "OSDP.MessageType", ftypes.UINT8,
                        { [0] = "Command", [1] = "Reply" }, base.NONE, 0x80)
local address = ProtoField.uint8("OSDP.Address", "PD Address", base.DEC, nil, 0x7f)
local length = ProtoField.uint16("OSDP.Length", "Packet Length", base.DEC)
local control = ProtoField.uint16("OSDP.Control", "Packet Control", base.DEC)

-- Entire payload
local payload = ProtoField.new("Payload", "OSDP.Payload", ftypes.BYTES)

-- SC sub entries
local scb = ProtoField.new("Secure Channel Block", "OSDP.SCB", ftypes.BYTES)
local sc_data = ProtoField.new("Enctrypted Data Block", "OSDP.SC_DATA", ftypes.BYTES)
local sc_mac = ProtoField.new("Message Authentication Code", "OSDP.SC_MAC", ftypes.BYTES)

-- Plaintext sub entries
local plaintext_id = ProtoField.uint8("OSDP.PlainText.ID", "Command/Reply ID", base.HEX)
local plaintext_data = ProtoField.new("Command/Reply Data", "OSDP.PlainText.Data", ftypes.BYTES)

-- Checksum/CRC16
local packet_check = ProtoField.new("CheckSum", "OSDP.CheckSum", ftypes.BYTES)

-- Helper methods
function ControlString(control_byte)
    local str = "SEQ: " .. tostring(bit32.band(control_byte, 0x3))
    if bit32.band(control_byte, 0x4) == 0x4 then
        str = str .. ", CRC16"
    else
        str = str .. ", CHECKSUM"
    end
    if bit32.band(control_byte, 0x8) == 0x8 then
        str = str .. ", SCB"
    end
    return str
end

-- Protocol definition
local osdp_protocol = Proto("OSDP", "Open Supervised Device Protocol")
osdp_protocol.fields = {
    header,
    mark, som, type, address, length, control,

    payload,
    scb, sc_data, sc_mac,
    plaintext_id, plaintext_data,
    packet_check
}

-- Protocol disector
function osdp_protocol.dissector(buffer, pinfo, tree)
    local packet_length = buffer:len()
    if length == 0 then return end

    local packet_header_len = 6
    local packet_mac_len = 4
    local packet_check_len = 1

    local subtree = tree:add(osdp_protocol, buffer(0, packet_length), "OSDP Packet")

    -- Packet header
    local header_subtree = subtree:add(header, buffer(0, packet_header_len))
    header_subtree:add(mark, buffer(0, 1))
    header_subtree:add(som, buffer(1, 1))
    header_subtree:add(type, buffer(2, 1))
    header_subtree:add(address, buffer(2, 1))
    header_subtree:add_le(length, buffer(3, 2))
    local control_byte = buffer(5, 1):uint()
    header_subtree:add(control, buffer(5, 1))
        :append_text(" (" .. ControlString(control_byte) .. ")")

    if bit32.band(control_byte, 0x04) == 0x04 then
        packet_check_len = 2
    end

    pinfo.cols.protocol = osdp_protocol.name
    local pd_address = bit32.band(buffer(2, 1):uint(), 0x7f)
    if bit32.band(buffer(2, 1):uint(), 0x80) == 0x80 then
        pinfo.cols.src = "PD[" .. tostring(pd_address) .. "]"
        pinfo.cols.dst = "CP"
    else
        pinfo.cols.src = "CP"
        pinfo.cols.dst = "PD[" .. tostring(pd_address) .. "]"
    end

    -- payload
    local offset = packet_header_len
    local payload_len = packet_length - packet_header_len - packet_check_len

    local payload_subtree = subtree:add(payload, buffer(offset, payload_len))
    if bit32.band(control_byte, 0x08) == 0x08 then
        -- Secure Control Block
        local scb_len = buffer(offset, 1):uint()
        local scb_type = buffer(offset + 1, 1):uint()

        payload_subtree:add(plaintext_id, buffer(offset + scb_len, 1))
        local scb_subtree = payload_subtree:add(scb, buffer(offset, scb_len))
            :append_text(" (SCS_" .. string.format("%x", scb_type) .. ")")

        offset = offset + scb_len
        payload_len = payload_len - scb_len

        -- Secure Data Block
        print("Secure Data Block: offset: " .. offset .. " payload_len: " .. payload_len)
        if scb_type >= 0x15 then
            if scb_type == 0x17 or scb_type == 0x18 then
                -- +1 and -1 below are to skip the Command/Response ID
                scb_subtree:add(sc_data, buffer(offset + 1, payload_len - 1))
            end
            scb_subtree:add(sc_mac, buffer(offset + payload_len - packet_mac_len, packet_mac_len))
        end
        pinfo.cols.info = "Seure Message"
    else
        -- Plain text block
        payload_subtree:add(plaintext_id, buffer(offset, 1))
        -- +1 and -1 below are to skip the Command/Response ID
        payload_subtree:add(plaintext_data, buffer(offset + 1, payload_len - 1))
        pinfo.cols.info = "Plaintext Message"
    end

    -- Packet check
    subtree:add(packet_check, buffer(packet_length - packet_check_len, packet_check_len))
end