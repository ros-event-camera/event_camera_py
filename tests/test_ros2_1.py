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

from rclpy.serialization import deserialize_message
from rosidl_runtime_py.utilities import get_message
from event_camera_py import Decoder
import numpy as np
import rosbag2_py


class BagReader:
    """Convenience class for reading ROS2 bags."""

    def __init__(self, bag_name, topics):
        bag_path = str(bag_name)
        storage_options, converter_options = self.get_rosbag_options(bag_path)
        self.reader = rosbag2_py.SequentialReader()
        self.reader.open(storage_options, converter_options)
        topic_types = self.reader.get_all_topics_and_types()
        self.type_map = {
            topic_types[i].name: topic_types[i].type for i in range(len(topic_types))
        }
        storage_filter = rosbag2_py.StorageFilter(topics=[topics])
        self.reader.set_filter(storage_filter)

    def has_next(self):
        return self.reader.has_next()

    def read_next(self):
        (topic, data, t_rec) = self.reader.read_next()
        msg_type = get_message(self.type_map[topic])
        msg = deserialize_message(data, msg_type)
        return (topic, msg, t_rec)

    def get_rosbag_options(self, path, serialization_format="cdr"):
        storage_options = rosbag2_py.StorageOptions(uri=path, storage_id="sqlite3")
        converter_options = rosbag2_py.ConverterOptions(
            input_serialization_format=serialization_format,
            output_serialization_format=serialization_format,
        )
        return storage_options, converter_options


class EventCounter:
    def __init__(self):
        self._num_on_events = 0
        self._num_off_events = 0
        self._num_rise_trig = 0
        self._num_fall_trig = 0
        self._sum_time = 0

    def add_cd_events(self, cd_events):
        if cd_events.shape[0] > 0:
            self._num_off_events += np.count_nonzero(cd_events["p"] == 0)
            self._num_on_events += np.count_nonzero(cd_events["p"] == 1)
            self._sum_time += np.sum(cd_events["t"])

    def add_trig_events(self, trig_events):
        if trig_events.shape[0] > 0:
            self._num_rise_trig += np.count_nonzero(trig_events["p"] == 0)
            self._num_fall_trig += np.count_nonzero(trig_events["p"] == 1)
            self._sum_time += np.sum(trig_events["t"])

    def check_count(
        self, sum_time, num_off_events, num_on_events, num_rise_trig, num_fall_trig
    ):
        assert sum_time == sum_time
        assert self._num_off_events == num_off_events
        assert self._num_on_events == num_on_events
        assert self._num_rise_trig == num_rise_trig
        assert self._num_fall_trig == num_fall_trig


def test_decode_msg():
    topic = "/event_camera/events"
    bag = BagReader("tests/test_events_ros2_1", topic)
    decoder = Decoder()
    counter = EventCounter()
    while bag.has_next():
        topic, msg, _ = bag.read_next()
        decoder.decode(msg)
        counter.add_cd_events(decoder.get_cd_events())
        counter.add_trig_events(decoder.get_ext_trig_events())

    counter.check_count(
        sum_time=6024891770230,
        num_off_events=238682,
        num_on_events=139584,
        num_rise_trig=18,
        num_fall_trig=18,
    )


def test_decode_bytes():
    topic = "/event_camera/events"
    bag = BagReader("tests/test_events_ros2_1", topic)
    decoder = Decoder()
    counter = EventCounter()
    while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        decoder.decode_bytes(
            msg.encoding, msg.width, msg.height, msg.time_base, msg.events.tobytes()
        )
        counter.add_cd_events(decoder.get_cd_events())
        counter.add_trig_events(decoder.get_ext_trig_events())

    counter.check_count(
        sum_time=6024891770230,
        num_off_events=238682,
        num_on_events=139584,
        num_rise_trig=18,
        num_fall_trig=18,
    )


def test_decode_until():
    topic = "/event_camera/events"
    bag = BagReader("tests/test_events_ros2_1", topic)
    decoder = Decoder()
    counter = EventCounter()
    frame_interval = 100000  # 100 usec
    t0 = 15024708  # first sensor time in data set
    frame_time = t0 + frame_interval
    while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        reachedTimeLimit = True
        while reachedTimeLimit:
            reachedTimeLimit, nextTime = decoder.decode_until(msg, frame_time)
            counter.add_cd_events(decoder.get_cd_events())
            counter.add_trig_events(decoder.get_ext_trig_events())
            while reachedTimeLimit and frame_time <= nextTime:
                frame_time += frame_interval
                print(f"frame time: {frame_time}, nextTime: {nextTime}")
    assert frame_time == 16824708
    counter.check_count(
        sum_time=6024891770230,
        num_off_events=238682,
        num_on_events=139584,
        num_rise_trig=18,
        num_fall_trig=18,
    )
