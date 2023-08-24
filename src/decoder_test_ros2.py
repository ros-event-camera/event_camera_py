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
import time
import numpy as np
from bag_reader_ros2 import BagReader
from event_camera_py import Decoder
from event_camera_py import UniqueDecoder


def print_stats(decoder, t0, t1):
    print(
        f"ON  events: {decoder.get_num_cd_on()}\n",
        f"OFF events: {decoder.get_num_cd_off()}",
    )
    print(
        f"RISE trigger events: {decoder.get_num_trigger_rising()} ",
        f"FALL trigger events: {decoder.get_num_trigger_falling()}",
    )
    n_cd = decoder.get_num_cd_on() + decoder.get_num_cd_off()
    n_trig = decoder.get_num_trigger_rising() + decoder.get_num_trigger_falling()
    dt = t1 - t0
    rate_cd = n_cd / dt
    rate_trig = n_trig / dt
    print(f"Total CD events: {n_cd} in time: {dt:3f}", f" rate: {rate_cd * 1e-6} Mevs")
    print(
        f"Total trigger events: {n_trig} in time: ",
        f"{dt:3f} rate: {rate_trig * 1e-6} Mevs",
    )


def test_decoder(fname, topic):
    bag = BagReader(fname, topic)
    decoder = Decoder()

    t0 = time.time()
    while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        decoder.decode(msg)
        _ = decoder.get_cd_events()
        _ = decoder.get_ext_trig_event_packets()
    t1 = time.time()
    print_stats(decoder, t0, t1)


def verify_unique(packets) -> bool:
    for p in (pp for pp in packets if pp.shape[0] > 0):
        idx = np.stack((p["x"], p["y"]), axis=1)
        _, c = np.unique(idx, return_counts=True, axis=0)
        num_with_duplicates = np.count_nonzero(c > 1)
        if num_with_duplicates > 0:
            print("bad packet:\n", p)
            print(f"has {num_with_duplicates} duplicates at:")
            return False
    return True


def test_unique_decoder(fname, topic):
    bag = BagReader(fname, topic)
    decoder = UniqueDecoder()
    t0 = time.time()
    while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        decoder.decode(msg)
        cd_event_packets = decoder.get_cd_event_packets()
        _ = decoder.get_ext_trig_event_packets()
        # print(cd_event_packets)
        if not verify_unique(cd_event_packets):
            raise Exception("packet indexes is not unique")

    t1 = time.time()
    print_stats(decoder, t0, t1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="read and decode events from bag.")
    parser.add_argument("--bag", required=True, help="bag file to read events from")
    parser.add_argument("--topic", help="ros topic to read", default="/event_camera/events")
    parser.add_argument("--type", default="regular", help="type (regular, unique).")
    args = parser.parse_args()
    if args.type == "unique":
        test_unique_decoder(args.bag, args.topic)
    else:
        test_decoder(args.bag, args.topic)
