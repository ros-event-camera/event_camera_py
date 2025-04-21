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

import os

# ------- hack to work around nosetest changing the module path
import os.path
import sys

sys.path = [os.path.abspath(os.path.dirname(__file__))] + sys.path
# ------- end of hack

from event_counter import EventCounter  # noqa: E402  (suppress flake8 error)
import test_verify  # noqa: E402  (suppress flake8 error)

from event_camera_py import Decoder  # noqa: I100, E402  (suppress flake8 error)
from event_camera_py import UniqueDecoder  # noqa: E402  (suppress flake8 error)

is_ros2 = os.environ['ROS_VERSION'] == '2'
if is_ros2:
    from bag_reader_ros2 import BagReader
else:
    from bag_reader_ros1 import BagReader


def test_decode_msg(verbose=False):
    bag = BagReader('tests/test_events_1', verbose)
    if verbose:
        print('Testing decode')
    decoder = Decoder()
    counter = EventCounter()
    for _, msg, _ in bag.read_messages(topics=['/event_camera/events']):
        decoder.decode(msg)
        counter.add_cd_events(decoder.get_cd_events())
        counter.add_trig_events(decoder.get_ext_trig_events())

    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=2885601049874,
        num_off_events=218291,
        num_on_events=125183,
        num_rise_trig=2078,
        num_fall_trig=2078,
    )


def test_decode_bytes(verbose=False):
    bag = BagReader('tests/test_events_1', verbose)
    if verbose:
        print('Testing decode_bytes')
    decoder = Decoder()
    counter = EventCounter()
    for _, msg, _ in bag.read_messages(topics=['/event_camera/events']):
        decoder.decode_bytes(
            msg.encoding,
            msg.width,
            msg.height,
            msg.time_base,
            msg.events.tobytes() if is_ros2 else msg.events,
        )
        counter.add_cd_events(decoder.get_cd_events())
        counter.add_trig_events(decoder.get_ext_trig_events())

    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=2885601049874,
        num_off_events=218291,
        num_on_events=125183,
        num_rise_trig=2078,
        num_fall_trig=2078,
    )


def test_decode_until(verbose=False):
    bag = BagReader('tests/test_events_1', verbose)
    if verbose:
        print('Testing decode_until')
    decoder = Decoder()
    counter = EventCounter()
    frame_interval = 100000  # 100 usec
    t0 = 7139845  # first sensor time in data set
    frame_time = t0 + frame_interval
    for _, msg, _ in bag.read_messages(topics=['/event_camera/events']):
        reachedTimeLimit = True
        while reachedTimeLimit:
            reachedTimeLimit, nextTime = decoder.decode_until(msg, frame_time)
            # foo = decoder.get_cd_events()
            counter.add_cd_events(decoder.get_cd_events())
            counter.add_trig_events(decoder.get_ext_trig_events())
            while reachedTimeLimit and frame_time <= nextTime:
                frame_time += frame_interval
    if verbose:
        print('final frame_time :: %d' % frame_time)

    assert frame_time == 9239845, 'bad frame time!'

    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=2885601049874,
        num_off_events=218291,
        num_on_events=125183,
        num_rise_trig=2078,
        num_fall_trig=2078,
    )


def test_unique(verbose=False):
    bag = BagReader('tests/test_events_1', verbose)
    if verbose:
        print('Testing unique')
    decoder = UniqueDecoder()
    counter = EventCounter()

    for _, msg, _ in bag.read_messages(topics=['/event_camera/events']):
        decoder.decode(msg)
        cd_packets = decoder.get_cd_event_packets()
        counter.add_cd_event_packets(cd_packets)
        assert test_verify.packet_is_unique(cd_packets)
        counter.add_trig_event_packets(decoder.get_ext_trig_event_packets())
    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=2885601049874,
        num_off_events=218291,
        num_on_events=125183,
        num_rise_trig=2078,
        num_fall_trig=2078,
    )


def test_unique_until(verbose=False):
    bag = BagReader('tests/test_events_1', verbose)
    if verbose:
        print('Testing unique_until')
    decoder = UniqueDecoder()
    counter = EventCounter()
    frame_interval = 100000  # 100 usec
    t0 = 7139845  # first sensor time in data set
    frame_time = t0 + frame_interval
    for _, msg, _ in bag.read_messages(topics=['/event_camera/events']):
        reachedTimeLimit = True
        while reachedTimeLimit:
            reachedTimeLimit, nextTime = decoder.decode_until(msg, frame_time)
            cd_packets = decoder.get_cd_event_packets()
            counter.add_cd_event_packets(cd_packets)
            assert test_verify.packet_is_unique(cd_packets)
            counter.add_trig_event_packets(decoder.get_ext_trig_event_packets())
            while reachedTimeLimit and frame_time <= nextTime:
                frame_time += frame_interval
    if verbose:
        print('final frame_time :: %d' % frame_time)
    assert frame_time == 9239845
    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=2885601049874,
        num_off_events=218291,
        num_on_events=125183,
        num_rise_trig=2078,
        num_fall_trig=2078,
    )


def test_find_first_sensor_time(verbose=True):
    bag = BagReader('tests/test_events_1', verbose)
    if verbose:
        print('Testing find_first_time_stamp')
    decoder = Decoder()
    for _, msg, _ in bag.read_messages(topics=['/event_camera/events']):
        ts = decoder.find_first_sensor_time(msg)
        assert ts is not None
        if verbose:
            print('first sensor time stamp: ', ts)
        assert ts == 7139840
        break


if __name__ == '__main__':
    test_decode_bytes(True)
    test_decode_msg(True)
    test_decode_until(True)
    test_unique(True)
    test_unique_until(True)
