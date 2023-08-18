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
from event_camera_py import UniqueDecoder
import numpy as np
import rosbag2_py


class BagReader:
    """Convenience class for reading ROS2 bags."""

    def __init__(self, bag_name, topics):
        self.bag_path = str(bag_name)
        storage_options, converter_options = self.get_rosbag_options(self.bag_path)
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

    def add_cd_event_packets(self, cd_event_packets):
        for p in cd_event_packets:
            self.add_cd_events(p)

    def add_trig_event_packets(self, trig_event_packets):
        for p in trig_event_packets:
            self.add_trig_events(p)

    def check_count(
        self, sum_time, num_off_events, num_on_events, num_rise_trig, num_fall_trig
    ):
        assert sum_time == sum_time
        assert self._num_off_events == num_off_events
        assert self._num_on_events == num_on_events
        assert self._num_rise_trig == num_rise_trig
        assert self._num_fall_trig == num_fall_trig

    def get_result_dict(self):
        return{
                "num_off_events": self._num_off_events,
                "num_on_events": self._num_on_events,
                "num_up_trig": self._num_up_trig,
                "num_down_trig": self._num_down_trig,
                "sum_time": self._sum_time,
                }

    def print_results(self):
        print(self.get_result_dict())


def test_decode_msg(verbose=False):
    topic = "/event_camera/events"
    bag = BagReader("tests/test_events_ros2_1", topic)
    if verbose:
        print("Testing decode")
        print("Opening bag :: "+bag.bag_path)
    decoder = Decoder()
    counter = EventCounter()
    while bag.has_next():
        topic, msg, _ = bag.read_next()
        decoder.decode(msg)
        counter.add_cd_events(decoder.get_cd_events())
        counter.add_trig_events(decoder.get_ext_trig_events())

    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=6024891770230,
        num_off_events=238682,
        num_on_events=139584,
        num_rise_trig=18,
        num_fall_trig=18,
    )


def test_decode_bytes(verbose=False):
    topic = "/event_camera/events"
    bag = BagReader("tests/test_events_ros2_1", topic)
    if verbose:
        print("Testing decode_bytes")
        print("Opening bag :: "+bag.bag_path)
    decoder = Decoder()
    counter = EventCounter()
    while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        decoder.decode_bytes(
            msg.encoding, msg.width, msg.height, msg.time_base, msg.events.tobytes()
        )
        counter.add_cd_events(decoder.get_cd_events())
        counter.add_trig_events(decoder.get_ext_trig_events())

    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=6024891770230,
        num_off_events=238682,
        num_on_events=139584,
        num_rise_trig=18,
        num_fall_trig=18,
    )


def test_decode_until(verbose=False):
    topic = "/event_camera/events"
    bag = BagReader("tests/test_events_ros2_1", topic)
    if verbose:
        print("Testing decode_until")
        print("Opening bag :: "+bag.bag_path)
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
    if verbose:
        print("frame_time :: %d" % frame_time)
    assert frame_time == 16824708

    if verbose:
        counter.print_results()

    counter.check_count(
        sum_time=6024891770230,
        num_off_events=238682,
        num_on_events=139584,
        num_rise_trig=18,
        num_fall_trig=18,
    )


def test_unique_until(verbose=False):
    topic = "/event_camera/events"
    bag = BagReader("tests/test_events_ros2_1", topic)
    if verbose:
        print("Testing unique_until")
        print("Opening bag :: "+bag.bag_path)
    decoder = UniqueDecoder()
    counter = EventCounter()
    frame_interval = 100000  # 100 usec
    t0 = 15024708  # first sensor time in data set
    frame_time = t0 + frame_interval
    while bag.has_next():
        topic, msg, t_rec = bag.read_next()
        reachedTimeLimit = True
        while reachedTimeLimit:
            reachedTimeLimit, nextTime = decoder.decode_until(msg, frame_time)
            cd_packets = decoder.get_cd_event_packets()
            counter.add_cd_event_packets(cd_packets)
            assert verify_unique(cd_packets)
            counter.add_trig_event_packets(decoder.get_ext_trig_event_packets())
            while reachedTimeLimit and frame_time <= nextTime:
                frame_time += frame_interval
    if verbose:
        print("frame_time :: %d" % frame_time)
    assert frame_time == 16824708
    if verbose:
        counter.print_results()
    counter.check_count(
        sum_time=6024891770230,
        num_off_events=238682,
        num_on_events=139584,
        num_rise_trig=18,
        num_fall_trig=18,
    )

if __name__ == "__main__":
    test_decode_msg(True)
    test_decode_array(True)
    test_decode_until(True)
    test_unique_until(True)
