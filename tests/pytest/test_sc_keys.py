#
#  Copyright (c) 2021 Siddharth Chandrasekaran <sidcha.dev@gmail.com>
#
#  SPDX-License-Identifier: Apache-2.0
#

from pyosdp import *

def test_set_new_scbk(utils):
    # Created single CP-PD pair
    pd = utils.create_pd('pd-101', utils.ks.new_key('pd-101'))
    cp = utils.create_cp([ 'pd-101' ], utils.ks.get_key('pd-101'))

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
    utils.ks.update_key('pd-101', new_key)

    # Stop CP and restart SC with new SCBK. PD should accept it
    cp.teardown()
    cp = utils.create_cp([ 'pd-101' ], utils.ks.get_key('pd-101'))
    assert cp.sc_wait(101)

    # Cleanup
    cp.teardown()
    pd.teardown()