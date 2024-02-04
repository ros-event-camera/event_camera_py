// -*-c++-*--------------------------------------------------------------------
// Copyright 2022 Bernd Pfrommer <bernd.pfrommer@gmail.com>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <event_camera_codecs/decoder.h>
#include <event_camera_py/accumulator.h>
#include <event_camera_py/accumulator_unique.h>
#include <event_camera_py/decoder.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <cstdint>
#include <iostream>
#include <string>

template <typename A>
void declare_decoder(pybind11::module & m, std::string typestr)
{
  using MyDecoder = Decoder<A>;
  const std::string pyName = typestr + "Decoder";
  pybind11::class_<MyDecoder>(m, pyName.c_str())
    .def(pybind11::init<>())
    .def("decode", &MyDecoder::decode, R"pbdoc(
        decode(event_packet) -> None

        Decode message

        :param event_packet: event packet
        :type event_packet:  event_camera_msgs/msgs/EventPacket
        )pbdoc")
    .def("decode_bytes", &MyDecoder::decode_bytes, R"pbdoc(
        decode_bytes(encoding, width, height, time_base, buffer) -> None

        Processes buffer of encoded events and updates state of the decoder.

        :param encoding: Encoding string (e.g. "evt3") as provided by the message.
        :type encoding: str
        :param width: sensor width in pixels
        :type width: int
        :param height: sensor height in pixels
        :type height: int
        :param time_base: Time base as provided by the message. Some codecs use it to
                          compute time stamps.
        :type time_base: int
        :param buffer: Buffer with encoded events to be processed, as provided by the message.
        :type buffer: bytes
        )pbdoc")
    .def("decode_array", &MyDecoder::decode_array, R"pbdoc(
        decode_array(encoding, width, height, time_base, buffer) -> None

        Processes buffer of encoded events and updates state of the decoder.

        :param encoding: Encoding string (e.g. "evt3") as provided by the message.
        :type encoding: str
        :param width: sensor width in pixels
        :type width: int
        :param height: sensor height in pixels
        :type height: int
        :param time_base: Time base as provided by the message. Some codecs use it to
                          compute time stamps.
        :type time_base: int
        :param buffer: Buffer with encoded events to be processed, as provided by the message.
        :type buffer: numpy array of dtype np.uint8_t
        )pbdoc")
    .def("decode_until", &MyDecoder::decode_until, R"pbdoc(
        decode_until(event_packet, until_time) -> tuple[Boolean, uint64_t]

        Processes message of encoded events and updates state of the decoder, but
        only until "until_time" is reached.

        :param event_packet: event packet
        :type event_packet:  event_camera_msgs/msgs/EventPacket
        :param until_time: sensor time (exclusive) up to which to process
        :type until_time: uint64_t

        :return tuple with flag (true if time limit has been reached) and
        time following time limit (only validif time limit has been reached!)
        :rtype height: tuple[boolean, uint64_t]
        )pbdoc")
    .def("get_start_time", &MyDecoder::get_start_time, R"pbdoc(
        get_start_time() -> uint64
        :return: start time offset for sensor time.
        :rtype: uint64
        )pbdoc")
    .def("get_cd_events", &MyDecoder::get_cd_events, R"pbdoc(
        get_cd_events() -> numpy.ndarray['EventCD']

        Fetches decoded change detected (CD) events. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.

        :return: array of detected events in the same format as the metavision SDK uses.
        :rtype: numpy.ndarray[EventCD], structured numpy array with fields 'x', 'y', 't', 'p'
        )pbdoc")
    .def("get_ext_trig_events", &MyDecoder::get_ext_trig_events, R"pbdoc(
        get_ext_trig_events() -> numpy.ndarray['EventExtTrig']

        Fetches decoded external trigger events. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.
        :return: array of trigger events in the same format as the metavision SDK uses.
        :rtype: numpy.ndarray[EventExtTrig], structured numpy array with fields 'p', 't', 'id'
        )pbdoc")
    .def("get_cd_event_packets", &MyDecoder::get_cd_event_packets, R"pbdoc(
        get_cd_event_packets() -> list[numpy.ndarray['EventCD']]

        Fetches decoded change detected (CD) event packets. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.

        :return: list of detected event packets
        :rtype: list[numpy.ndarray[EventCD]], list of structured numpy array with fields 'x', 'y', 't', 'p'
        )pbdoc")
    .def("get_ext_trig_event_packets", &MyDecoder::get_ext_trig_event_packets, R"pbdoc(
        get_ext_trig_event_packets() -> list[numpy.ndarray['EventExtTrig']]

        Fetches decoded external trigger event packets. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.

        :return: list of detected event packets
        :rtype: list[numpy.ndarray[EventExtTrig]], list of structured numpy array with fields 'p', 't', 'id'
        )pbdoc")
    .def("get_num_cd_on", &MyDecoder::get_num_cd_on, R"pbdoc(
        get_num_cd_on() -> int
        :return: cumulative number of ON events.
        :rtype: int
        )pbdoc")
    .def("get_num_cd_off", &MyDecoder::get_num_cd_off, R"pbdoc(
        get_num_cd_off() -> int
        :return: cumulative number of OFF events.
        :rtype: int
        )pbdoc")
    .def("get_num_trigger_rising", &MyDecoder::get_num_trigger_rising, R"pbdoc(
        get_num_trigger_rising() -> int
        :return: cumulative number of rising edge external trigger events.
        :rtype: int
        )pbdoc")
    .def("get_num_trigger_falling", &MyDecoder::get_num_trigger_falling, R"pbdoc(
        get_num_trigger_falling() -> int
        :return: cumulative number of falling edge external trigger events.
        :rtype: int
        )pbdoc");
}

PYBIND11_MODULE(_event_camera_py, m)
{
  pybind11::options options;
  options.disable_function_signatures();
  m.doc() = R"pbdoc(
        Plugin for processing event_camera_msgs in python
    )pbdoc";

  PYBIND11_NUMPY_DTYPE(EventCD, x, y, p, t);
  PYBIND11_NUMPY_DTYPE(EventExtTrig, p, t, id);

  declare_decoder<Accumulator>(m, "");
  declare_decoder<AccumulatorUnique>(m, "Unique");
}
