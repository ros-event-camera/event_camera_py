# -----------------------------------------------------------------------------
# Copyright 2023 Bernd Pfrommer <bernd.pfrommer@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import rosbag
import numpy as np
from event_camera_py import Decoder

def test_decode(bag, topic):
    decoder = Decoder()
    num_on_events = 0
    num_off_events = 0
    num_up_trig = 0
    num_down_trig = 0
    sum_time = 0
    for topic, msg, t in bag.read_messages(topics=topic):
        decoder.decode(msg)
        cd_events = decoder.get_cd_events()
        trig_events = decoder.get_ext_trig_events()
        if cd_events.shape[0] > 0:
            num_off_events += np.count_nonzero(cd_events['p'] == 0)
            num_on_events += np.count_nonzero(cd_events['p'] == 1)
            sum_time += np.sum(cd_events['t'])
        if trig_events.shape[0] > 0:
            num_up_trig += np.count_nonzero(trig_events['p'] == 0)
            num_down_trig += np.count_nonzero(trig_events['p'] == 1)
            sum_time += np.sum(trig_events['t'])

    return {
            "num_off_events": num_off_events,
            "num_on_events": num_on_events,
            "num_up_trig": num_up_trig,
            "num_down_trig": num_down_trig,
            "sum_time": sum_time,
           }

def test_decode_bytes(bag, topic):
    decoder = Decoder()
    num_on_events = 0
    num_off_events = 0
    num_up_trig = 0
    num_down_trig = 0
    sum_time = 0
    for topic, msg, t in bag.read_messages(topics=topic):
        decoder.decode_bytes(msg.encoding, msg.width, msg.height,
                             msg.time_base, msg.events)
        cd_events = decoder.get_cd_events()
        trig_events = decoder.get_ext_trig_events()
        if cd_events.shape[0] > 0:
            num_off_events += np.count_nonzero(cd_events['p'] == 0)
            num_on_events += np.count_nonzero(cd_events['p'] == 1)
            sum_time += np.sum(cd_events['t'])
        if trig_events.shape[0] > 0:
            num_up_trig += np.count_nonzero(trig_events['p'] == 0)
            num_down_trig += np.count_nonzero(trig_events['p'] == 1)
            sum_time += np.sum(trig_events['t'])

    return {
            "num_off_events": num_off_events,
            "num_on_events": num_on_events,
            "num_up_trig": num_up_trig,
            "num_down_trig": num_down_trig,
            "sum_time": sum_time,
           }

def test_decode_array(bag, topic):
    decoder = Decoder()
    num_on_events = 0
    num_off_events = 0
    num_up_trig = 0
    num_down_trig = 0
    sum_time = 0
    for topic, msg, t in bag.read_messages(topics=topic):
        decoder.decode_array(msg.encoding, msg.width, msg.height,
                             msg.time_base, np.frombuffer(msg.events, dtype=np.uint8))
        cd_events = decoder.get_cd_events()
        trig_events = decoder.get_ext_trig_events()
        if cd_events.shape[0] > 0:
            num_off_events += np.count_nonzero(cd_events['p'] == 0)
            num_on_events += np.count_nonzero(cd_events['p'] == 1)
            sum_time += np.sum(cd_events['t'])
        if trig_events.shape[0] > 0:
            num_up_trig += np.count_nonzero(trig_events['p'] == 0)
            num_down_trig += np.count_nonzero(trig_events['p'] == 1)
            sum_time += np.sum(trig_events['t'])

    return {
            "num_off_events": num_off_events,
            "num_on_events": num_on_events,
            "num_up_trig": num_up_trig,
            "num_down_trig": num_down_trig,
            "sum_time": sum_time,
           }

def test_readbag(verbose=False):
    if verbose:
        print("Starting ROS1 read test")

    topic = '/event_camera/events'
    bag_name = 'tests/test_events_ros1_1.bag'
    if verbose:
        print("Opening bag :: " + bag_name)

    bag = rosbag.Bag('tests/test_events_ros1_1.bag')

    tests = {
            "decode": test_decode(bag, topic),
            "decode_bytes": test_decode_bytes(bag, topic),
            "decode_array": test_decode_array(bag, topic),
            }

    for method, result in tests.items():
        if verbose:
            print("Results :: [%s]" % method)
            print(result)

        assert(result['num_off_events'] == 218291)
        assert(result['num_on_events'] == 125183)
        assert(result['num_up_trig'] == 2078)
        assert(result['num_down_trig'] == 2079)
        assert(result['sum_time'] == 2885601049874)

if __name__ == "__main__":
    test_readbag(True)
