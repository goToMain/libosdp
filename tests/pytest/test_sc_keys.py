#
#  Copyright (c) 2021-2023 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

from testlib import *

def test_set_new_scbk(utils):
    # Create single CP-PD pair
    pd_info = PDInfo(101, utils.ks.new_key('sc-keys-pd'), name='sc-keys-pd')
    pd = utils.create_pd(pd_info)
    cp = utils.create_cp([ pd_info ], sc_wait=True)

    # Set a new SCBK from the CP side and verify whether it was received
    # by the PD as we intended.
    new_key = utils.ks.gen_key()
    keyset_cmd = {
        'command': Command.Keyset,
        'type': 1,
        'data': new_key
    }
    assert cp.send_command(101, keyset_cmd)
    cmd = pd.get_command()
    assert cmd == keyset_cmd
    utils.ks.update_key('sc-keys-pd', new_key)

    # Stop CP and restart SC with new SCBK. PD should accept it
    cp.teardown()
    pd_info = PDInfo(101, utils.ks.get_key('sc-keys-pd'), name='sc-keys-pd')
    cp = utils.create_cp([ pd_info ])
    assert cp.sc_wait(101)

    # Cleanup
    cp.teardown()
    pd.teardown()
