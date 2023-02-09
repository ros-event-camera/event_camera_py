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
from event_array_py import Decoder


def test_readbag():
    topic = '/event_camera/events'
    bag = rosbag.Bag('tests/test_events_ros1_1.bag')
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

    assert(num_off_events == 218291)
    assert(num_on_events == 125183)
    assert(num_up_trig == 2078)
    assert(num_down_trig == 2079)
    assert(sum_time == 2885601049874)
