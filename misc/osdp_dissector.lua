--
--  Copyright (c) 2024 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
--
--  SPDX-License-Identifier: Apache-2.0

local malformed_error = ProtoExpert.new("Error.Malformed", "Malformed Packet", expert.group.MALFORMED, expert.severity.ERROR)

-- Header
local header = ProtoField.new("OSDP Header", "OSDP.Header", ftypes.BYTES)
local mark = ProtoField.uint8("OSDP.Mark", "Mark byte", base.HEX)
local som = ProtoField.uint8("OSDP.SOM", "Start of message", base.HEX)
local type = ProtoField.new("Message Type", "OSDP.MessageType", ftypes.UINT8, { [0] = "Command", [1] = "Reply" }, base.NONE, 0x80)
local address = ProtoField.new("PD Address", "OSDP.Address", ftypes.UINT8, nil, base.DEC, 0x7f)
local length = ProtoField.uint16("OSDP.Length", "Packet Length", base.DEC)

local control = ProtoField.new("Packet Control", "OSDP.Control", ftypes.UINT8, nil, base.HEX)
local control_sequence = ProtoField.new("Sequence", "OSDP.Control.Sequence", ftypes.UINT8, nil, base.DEC, 0x03)
local control_check = ProtoField.new("Integrity Check", "OSDP.Control.IC", ftypes.UINT8, { [0] = "Checksum", [1] = "CRC-16" }, base.NONE, 0x04)
local control_scb = ProtoField.new("Secure Control Block", "OSDP.Control.SCB", ftypes.UINT8, { [0] = "Absent", [1] = "Present" }, base.NONE, 0x08)
local control_trace = ProtoField.new("Capture Type", "OSDP.Control.Trace", ftypes.UINT8, { [0] = "Original", [1] = "Mangled" }, base.NONE, 0x80)

-- Entire payload
local payload = ProtoField.new("OSDP Payload", "Payload", ftypes.BYTES)

-- SC sub entries
local scb = ProtoField.new("Secure Channel Block", "SCB", ftypes.BYTES)
local sc_data = ProtoField.new("Enctrypted Data Block", "SC_DATA", ftypes.BYTES)
local sc_mac = ProtoField.new("Message Authentication Code", "SC_MAC", ftypes.BYTES)

-- Plaintext sub entries
local plaintext_id = ProtoField.uint8("PlainText.ID", "Command/Reply ID", base.HEX)
local plaintext_data = ProtoField.new("Command/Reply Data", "PlainText.Data", ftypes.BYTES)

-- Checksum/CRC16
local packet_check = ProtoField.new("CheckSum", "CheckSum", ftypes.BYTES)

-- Protocol definition
local osdp_protocol = Proto("OSDP", "Open Supervised Device Protocol")
osdp_protocol.fields = {
    header,
    mark, som, type, address, length,
    control, control_sequence, control_check, control_scb, control_trace,

    payload,
    scb, sc_data, sc_mac,
    plaintext_id, plaintext_data,
    packet_check
}

-- Protocol disector
function osdp_protocol.dissector(buffer, pinfo, tree)
    local packet_length = buffer:len()
    if length == 0 then
        tree:add_proto_expert_info(malformed_error)
        return
    end

    local packet_mac_len = 4
    local packet_check_len = 1
    local has_mark = false;
    local first_byte = buffer(0, 1):uint()
    if first_byte == 0xFF then
        has_mark = true;
    else
        if first_byte ~= 0x53 then
            tree:add_proto_expert_info(malformed_error)
            return
        end
    end
    local packet_header_len = 5
    local pos = 0

    if has_mark then
        packet_header_len = 6
        pos = 1
    end

    local control_byte = buffer(pos + 4, 1):uint()
    local has_crc16 = bit32.band(control_byte, 0x04) == 0x04
    local has_scb = bit32.band(control_byte, 0x08) == 0x08
    local trace_mangled = bit32.band(control_byte, 0x80) == 0x80

    if has_crc16 then
        packet_check_len = 2
    end
    if trace_mangled then
        packet_check_len = 0
    end

    local subtree = tree:add(osdp_protocol, buffer(0, packet_length), "OSDP Packet")

    -- Packet header
    local header_subtree = subtree:add(header, buffer(0, packet_header_len))
    if has_mark then
        header_subtree:add(mark, buffer(0, 1))
    end
    header_subtree:add(som, buffer(pos + 0, 1))
    header_subtree:add(type, buffer(pos + 1, 1))
    header_subtree:add(address, buffer(pos + 1, 1))
    header_subtree:add_le(length, buffer(pos + 2, 2))

    local control_subtree = header_subtree:add(control, buffer(pos + 4, 1))
    control_subtree:add(control_sequence, buffer(pos + 4, 1))
    control_subtree:add(control_check, buffer(pos + 4, 1))
    control_subtree:add(control_scb, buffer(pos + 4, 1))
    control_subtree:add(control_trace, buffer(pos + 4, 1))

    pinfo.cols.protocol = osdp_protocol.name
    local pd_address = bit32.band(buffer(pos + 1, 1):uint(), 0x7f)
    local is_cmd = bit32.band(buffer(pos + 1, 1):uint(), 0x80) ~= 0x80
    if is_cmd then
        pinfo.cols.src = "CP"
        pinfo.cols.dst = "PD[" .. tostring(pd_address) .. "]"
    else
        pinfo.cols.src = "PD[" .. tostring(pd_address) .. "]"
        pinfo.cols.dst = "CP"
    end

    -- payload
    local offset = packet_header_len
    local payload_len = packet_length - packet_header_len - packet_check_len

    local payload_subtree = subtree:add(payload, buffer(offset, payload_len))
    if has_scb then
        -- Secure Control Block
        local scb_len = buffer(offset, 1):uint()
        local scb_type = buffer(offset + 1, 1):uint()
        local scs_type_name = "SCS_" .. string.format("%x", scb_type)

        local scb_subtree = payload_subtree:add(scb, buffer(offset, scb_len))
        payload_subtree:add(plaintext_id, buffer(offset + scb_len, 1))

        -- +1 and -1 below are to skip the Command/Response ID
        offset = offset + scb_len + 1
        payload_len = payload_len - scb_len - 1
        local info = "Seure Message"
        if scb_type < 0x15 then
            info = "SC Handshake"
        end
        if trace_mangled then
            -- This message was mangled by data tracer to have SCB but no
            -- encrypted data so we add them as is.
            payload_subtree:add(plaintext_data, buffer(offset, payload_len))
            info = "Decrypted " .. info
        else
            -- Secure Data Block
            if scb_type >= 0x15 then
                if scb_type == 0x17 or scb_type == 0x18 then
                    scb_subtree:add(sc_data, buffer(offset, payload_len))
                    info = info .. " with Data"
                end
                scb_subtree:add(sc_mac, buffer(offset + payload_len - packet_mac_len, packet_mac_len))
            end
        end
        pinfo.cols.info = info .. " (" .. scs_type_name .. ")"
    else
        -- Plain text block
        payload_subtree:add(plaintext_id, buffer(offset, 1))
        -- +1 and -1 below are to skip the Command/Response ID
        payload_subtree:add(plaintext_data, buffer(offset + 1, payload_len - 1))
        pinfo.cols.info = "Plaintext Message"
    end

    -- Packet check
    if packet_check_len > 0 then
        subtree:add(packet_check, buffer(packet_length - packet_check_len, packet_check_len))
    end
end