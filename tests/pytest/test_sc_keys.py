#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

from pyosdp import *

def create_cp(names, key, flag_list=[]):
    pd_info_list = []
    for name in names:
        addr = int(name.split("-")[1])
        pd_info_list.append(PDInfo(addr, scbk=key, flags=flag_list, name=name))
    cp = ControlPanel(pd_info_list, log_level=LogLevel.Debug)
    cp.start()
    return cp

def create_pd(name, key, flag_list=[]):
    addr = int(name.split("-")[1])
    pd_info = PDInfo(addr, scbk=key, flags=flag_list, name=name)
    pd = PeripheralDevice(pd_info, PDCapabilities(), log_level=LogLevel.Debug)
    pd.start()
    return pd

def test_set_new_scbk():
    ks = KeyStore()
    pd = create_pd('pd-101', ks.new_key('pd-101'))
    cp = create_cp([ 'pd-101' ], ks.get_key('pd-101'))
    new_key = KeyStore.gen_key()
    keyset_cmd = {
        'command': Command.Keyset,
        'type': 1,
        'data': new_key
    }
    assert cp.send_command(101, keyset_cmd)
    cmd = pd.get_command()
    assert cmd == keyset_cmd
    ks.update_key('pd-101', new_key)

    # Stop CP and restart SC with new SCBK. PD should accept it
    cp.teardown()
    cp = create_cp([ 'pd-101' ], ks.get_key('pd-101'))
    assert cp.sc_wait(101)

    # Cleanup
    cp.teardown()
    pd.teardown()