# -----------------------------------------------------------------------------
# Copyright 2025 Bernd Pfrommer <bernd.pfrommer@gmail.com>
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

from event_camera_py import Decoder  # noqa: I100, E402  (suppress flake8 error)

is_ros2 = os.environ['ROS_VERSION'] == '2'
if is_ros2:
    from bag_reader_ros2 import BagReader
else:
    from bag_reader_ros1 import BagReader

bag_file = 'tests/test_events_2'
topic = '/event_camera/events'


def test_decode_until(verbose=False):
    bag = BagReader(bag_file, verbose)
    if verbose:
        print('Testing decode_until')
    decoder = Decoder()
    counter = EventCounter()
    frame_interval = 100000  # 100 usec
    frame_time = None
    frame_count = 0
    for _, msg, _ in bag.read_messages(topics=[topic]):
        reachedTimeLimit = True
        while reachedTimeLimit:
            decoder.decode(msg)
            if frame_time is None:
                frame_time = decoder.find_first_sensor_time(msg) + frame_interval
            reachedTimeLimit, nextTime = decoder.decode_until(msg, frame_time)
            counter.add_cd_events(decoder.get_cd_events())
            counter.add_trig_events(decoder.get_ext_trig_events())
            while reachedTimeLimit and frame_time <= nextTime:
                frame_time += frame_interval
                frame_count += 1
    if verbose:
        print('number of frames found :: %d' % frame_count)
        print('final frame_time :: %d' % frame_time)

    assert frame_time == 1744227145732712, 'bad frame time!'
    assert frame_count == 9, 'bad frame count!'
    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=335555680277,
        num_off_events=360223,
        num_on_events=211043,
        num_rise_trig=0,
        num_fall_trig=0,
    )


def test_find_first_sensor_time(verbose=True):
    bag = BagReader(bag_file, verbose)
    if verbose:
        print('Testing find_first_time_stamp')
    decoder = Decoder()
    for _, msg, _ in bag.read_messages(topics=[topic]):
        ts = decoder.find_first_sensor_time(msg)
        assert ts is not None
        if verbose:
            print('first sensor time stamp :: ', ts)
        assert ts == 1744227144732712
        break


if __name__ == '__main__':
    test_find_first_sensor_time(True)
    test_decode_until(True)
