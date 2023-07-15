#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# Copyright 2022 Bernd Pfrommer <bernd.pfrommer@gmail.com>
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
#
"""Test decoder on data."""

import argparse
import rosbag
import time
from event_camera_py import Decoder


def test_decoder(fname, topic):
    bag = rosbag.Bag(fname)
    decoder = Decoder()
    t0 = time.time()
    for topic, msg, t in bag.read_messages(topics=topic):
        decoder.decode_bytes(msg.encoding, msg.width, msg.height,
                             msg.time_base, msg.events)
        cd_events = decoder.get_cd_events()
        print(cd_events)
        trig_events = decoder.get_ext_trig_events()
        print(trig_events)
    t1 = time.time()
    bag.close()
    print(f"ON  events: {decoder.get_num_cd_on()}\n",
          f"OFF events: {decoder.get_num_cd_off()}")
    print(f"RISE trigger events: {decoder.get_num_trigger_rising()} ",
          f"FALL trigger events: {decoder.get_num_trigger_falling()}")
    n_cd = decoder.get_num_cd_on() + decoder.get_num_cd_off()
    n_trig = decoder.get_num_trigger_rising() + \
        decoder.get_num_trigger_falling()
    dt = t1 - t0
    rate_cd = n_cd / dt
    rate_trig = n_trig / dt
    print(f"Total CD events: {n_cd} in time: {dt:3f} rate: {rate_cd * 1e-6} Mevs")
    print(f"Total trigger events: {n_trig} in time: {dt:3f}",
          f" rate: {rate_trig * 1e-6} Mevs")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='read and decode events from bag.')
    parser.add_argument('--bag', '-b', action='store', default=None,
                        required=True, help='bag file to read events from')
    parser.add_argument('--topic', help='Event topic to read',
                        default='/event_camera/events', type=str)
    args = parser.parse_args()

    test_decoder(args.bag, args.topic)
