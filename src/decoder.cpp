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
    .def(pybind11::init<>(), R"pbdoc(
        Decoder() -> None

        Instantiates decoder object.
        )pbdoc")
    .def("decode", &MyDecoder::decode, R"pbdoc(
        decode(msg) -> None

        Decode event message. After calling decode() the decoded events
        can be accessed via get_cd_events() or get_ext_trig_events(). This
        call updates the state of the decoder, so no messages should be skipped.

        :param msg: event packet msg to decode
        :type msg:  event_camera_msgs/msgs/EventPacket
        )pbdoc")
    .def("decode_bytes", &MyDecoder::decode_bytes, R"pbdoc(
        decode_bytes(encoding, width, height, time_base, buffer) -> None

        Processes buffer of encoded events and updates state of the decoder.
        Works similar to decode() but is less convenient to use.

        :param encoding: Encoding string (e.g. "evt3") as provided by the message.
        :type encoding: str
        :param width: sensor width in pixels
        :type width: uint16_t
        :param height: sensor height in pixels
        :type height: uint16_t
        :param time_base: Time base as provided by the message. Some codecs use it to
                          compute time stamps.
        :type time_base: uint64_t
        :param buffer: Buffer with encoded events to be processed, as provided by the message.
        :type buffer: bytes
        )pbdoc")
    .def("decode_array", &MyDecoder::decode_array, R"pbdoc(
        decode_array(encoding, width, height, time_base, buffer) -> None

        Processes buffer of encoded events and updates state of the decoder.

        :param encoding: Encoding string (e.g. "evt3") as provided by the message.
        :type encoding: str
        :param width: sensor width in pixels
        :type width: uint16_t
        :param height: sensor height in pixels
        :type height: uint16_t
        :param time_base: Time base as provided by the message. Some codecs use it to
                          compute time stamps.
        :type time_base: uint64_t
        :param buffer: Buffer with encoded events to be processed, as provided by the message.
        :type buffer: numpy.ndarray dtype uint8_t
        )pbdoc")
    .def("decode_until", &MyDecoder::decode_until, R"pbdoc(
        decode_until(msg, until_time) -> tuple[Boolean, uint64_t]

        Processes message of encoded events and updates state of the decoder, but
        only until "until_time" is reached.

        :param msg: event packet message to decode
        :type msg:  event_camera_msgs/msgs/EventPacket
        :param until_time: sensor time (exclusive) up to which to process
        :type until_time: uint64_t
        :return: tuple with flag (true if time limit has been reached) and the
          time following time limit (only valid if time limit has been reached!)
        :rtype: tuple[boolean, uint64_t]
        )pbdoc")
    .def("get_start_time", &MyDecoder::get_start_time, R"pbdoc(
        get_start_time() -> uint64
        
        This function is useful for sensors that provide absolute time stamps,
        for example libcaer based devices. The time stamps of the decoded events
        starts at zero, but the absolut event time in nanoseconds since begin of the epoch
        can be recovered by adding the start time.

        :return: start time offset for sensor time, in nanoseconds
        :rtype: uint64_t
        )pbdoc")
    .def("get_cd_events", &MyDecoder::get_cd_events, R"pbdoc(
        get_cd_events() -> numpy.ndarray['EventCD']

        Fetches decoded change detected (CD) events. Will clear out decoded events, to be
        called only *once*. If not called, events will be lost the next time decode() is called.
        The returned structured numpy array has fields 'x', 'y', 't', 'p'. Event time is a signed
        32 bit integer representing microseconds.

        :return: array of detected events in the same format as the metavision SDK uses.
        :rtype: numpy.ndarray[EventCD]
        )pbdoc")
    .def("get_ext_trig_events", &MyDecoder::get_ext_trig_events, R"pbdoc(
        get_ext_trig_events() -> numpy.ndarray['EventExtTrig']

        Fetches decoded external trigger events. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.
        The returned structured numpy array has fields 'p', 't' and 'id'.

        :return: array of trigger events in the same format as the metavision SDK uses.
        :rtype: numpy.ndarray[EventExtTrig]
        )pbdoc")
    .def("get_cd_event_packets", &MyDecoder::get_cd_event_packets, R"pbdoc(
        get_cd_event_packets() -> list[numpy.ndarray['EventCD']]

        *Only used in combination with Unique Decoder!*
        Fetches decoded change detected (CD) event packets. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.
        The returned list has elements that are structured numpy arrays. You are guaranteed that
        the events within each list element do not overlap spatially, i.e. the same (x, y) is not
        accessed more than once.
        
        :return: list of detected event packets
        :rtype: list[numpy.ndarray[EventCD]]
        )pbdoc")
    .def("get_ext_trig_event_packets", &MyDecoder::get_ext_trig_event_packets, R"pbdoc(
        get_ext_trig_event_packets() -> list[numpy.ndarray['EventExtTrig']]

        *Only used in combination with Unique Decoder!*
        Fetches decoded external trigger event packets. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.
        The returned list has elements that are structured numpy arrays. You are guaranteed that
        the events within each list element do not overlap spatially, i.e. the same (x, y) is not
        accessed more than once.

        :return: list of detected event packets
        :rtype: list[numpy.ndarray[EventExtTrig]]
        )pbdoc")
    .def("get_num_cd_on", &MyDecoder::get_num_cd_on, R"pbdoc(
        get_num_cd_on() -> uint64_t

        :return: cumulative number of ON events since beginning of decoding
        :rtype: uint64_t
        )pbdoc")
    .def("get_num_cd_off", &MyDecoder::get_num_cd_off, R"pbdoc(
        get_num_cd_off() -> uint64_t

        :return: cumulative number of OFF events.
        :rtype: uint64_t
        )pbdoc")
    .def("get_num_trigger_rising", &MyDecoder::get_num_trigger_rising, R"pbdoc(
        get_num_trigger_rising() -> uint64_t
    
        :return: cumulative number of rising edge external trigger events.
        :rtype: uint64_t
        )pbdoc")
    .def("get_num_trigger_falling", &MyDecoder::get_num_trigger_falling, R"pbdoc(
        get_num_trigger_falling() -> uint64_t
    
        :return: cumulative number of falling edge external trigger events.
        :rtype: uint64_t
        )pbdoc");
}

PYBIND11_MODULE(_event_camera_py, m)
{
  pybind11::options options;
  // options.disable_function_signatures();
  m.doc() = R"pbdoc(
        Plugin for processing event_camera_msgs in python
    )pbdoc";

  PYBIND11_NUMPY_DTYPE(EventCD, x, y, p, t);
  PYBIND11_NUMPY_DTYPE(EventExtTrig, p, t, id);

  declare_decoder<Accumulator>(m, "");
  declare_decoder<AccumulatorUnique>(m, "Unique");
}
