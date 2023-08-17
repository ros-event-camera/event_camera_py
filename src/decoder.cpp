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
#include <event_camera_py/decoder.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <cstdint>
#include <iostream>
#include <string>

template <class T>
static T get_attr(pybind11::object msg, const std::string & name)
{
  if (!pybind11::hasattr(msg, name.c_str())) {
    throw std::runtime_error("event packet has no " + name + " field");
  }
  return (pybind11::getattr(msg, name.c_str()).cast<T>());
}

Decoder::Decoder() {}

void Decoder::decode(pybind11::object msg)
{
  pybind11::array_t<uint8_t> events = get_attr<pybind11::array_t<uint8_t>>(msg, "events");
  do_full_decode(
    get_attr<std::string>(msg, "encoding"), get_attr<uint32_t>(msg, "width"),
    get_attr<uint32_t>(msg, "height"), get_attr<uint64_t>(msg, "time_base"), events.data(),
    events.size());
}

void Decoder::decode_bytes(
  const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
  pybind11::bytes events)
{
  const uint8_t * buf = reinterpret_cast<const uint8_t *>(PyBytes_AsString(events.ptr()));
  do_full_decode(encoding, width, height, timeBase, buf, PyBytes_Size(events.ptr()));
}

void Decoder::decode_array(
  const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
  pybind11::array_t<uint8_t> events)
{
  if (events.ndim() != 1 || !pybind11::isinstance<pybind11::array_t<uint8_t>>(events)) {
    throw std::runtime_error("Input events must be 1-D numpy array of type uint8");
  }
  const uint8_t * buf = reinterpret_cast<const uint8_t *>(events.data());
  do_full_decode(encoding, width, height, timeBase, buf, events.size());
}

std::tuple<bool, uint64_t> Decoder::decode_until(pybind11::object msg, uint64_t untilTime)
{
  auto decoder = initialize_decoder(
    get_attr<std::string>(msg, "encoding"), get_attr<uint32_t>(msg, "width"),
    get_attr<uint32_t>(msg, "height"));
  const uint64_t timeBase = get_attr<uint64_t>(msg, "time_base");
  reset_stored_events();
  pybind11::array_t<uint8_t> events = get_attr<pybind11::array_t<uint8_t>>(msg, "events");
  uint64_t nextTime{0};
  const bool reachedTimeLimit =
    decoder->decodeUntil(events.data(), events.size(), this, untilTime, timeBase, &nextTime);
  return (std::tuple<bool, uint64_t>({reachedTimeLimit, nextTime}));
}

pybind11::array_t<EventCD> Decoder::get_cd_events()
{
  if (cdEvents_) {
    auto p = cdEvents_;
    auto cap =
      pybind11::capsule(p, [](void * v) { delete reinterpret_cast<std::vector<EventCD> *>(v); });
    cdEvents_ = 0;  // clear out
    return (pybind11::array_t<EventCD>(p->size(), p->data(), cap));
  }
  return (pybind11::array_t<EventCD>());
}

pybind11::array_t<EventExtTrig> Decoder::get_ext_trig_events()
{
  if (extTrigEvents_) {
    auto p = extTrigEvents_;
    auto cap = pybind11::capsule(
      p, [](void * v) { delete reinterpret_cast<std::vector<EventExtTrig> *>(v); });
    extTrigEvents_ = 0;  // clear out
    return (pybind11::array_t<EventExtTrig>(p->size(), p->data(), cap));
  }
  return (pybind11::array_t<EventExtTrig>());
}

void Decoder::eventCD(uint64_t sensor_time, uint16_t ex, uint16_t ey, uint8_t polarity)
{
  cdEvents_->push_back(EventCD(ex, ey, polarity, sensor_time));
  maxSizeCD_ = std::max(cdEvents_->size(), maxSizeCD_);
  numCDEvents_[std::min(polarity, uint8_t(1))]++;
}

void Decoder::eventExtTrigger(uint64_t sensor_time, uint8_t edge, uint8_t id)
{
  extTrigEvents_->push_back(EventExtTrig(
    static_cast<int16_t>(edge), static_cast<int64_t>(sensor_time), static_cast<int16_t>(id)));
  maxSizeExtTrig_ = std::max(extTrigEvents_->size(), maxSizeExtTrig_);
  numExtTrigEvents_[std::min(edge, uint8_t(1))]++;
}

void Decoder::do_full_decode(
  const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
  const uint8_t * buf, size_t bufSize)
{
  auto decoder = initialize_decoder(encoding, width, height);
  decoder->setTimeBase(timeBase);

  reset_stored_events();
  decoder->decode(buf, bufSize, this);
}

event_camera_codecs::Decoder<event_camera_codecs::EventPacket, Decoder> *
Decoder::initialize_decoder(const std::string & encoding, uint32_t width, uint32_t height)
{
  auto decoder = decoderFactory_.getInstance(encoding, width, height);
  if (!decoder) {
    throw(std::runtime_error("no decoder for encoding " + encoding));
  }
  decoder->setTimeMultiplier(1);  // report in usecs instead of nanoseconds
  return (decoder);
}

void Decoder::reset_stored_events()
{
  delete cdEvents_;  // in case events have not been picked up
  cdEvents_ = new std::vector<EventCD>();
  delete extTrigEvents_;  // in case events have not been picked up
  extTrigEvents_ = new std::vector<EventExtTrig>();
  // TODO(Bernd): use hack here to avoid initializing the memory
  cdEvents_->reserve(maxSizeCD_);
  extTrigEvents_->reserve(maxSizeExtTrig_);
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

  pybind11::class_<Decoder>(
    m, "Decoder",
    R"pbdoc(
        Class to decode event_camera_msgs in python. The decoder
        keeps state inbetween calls to decode(). After calling decode()
        the events must be read via get_cd_events() before calling
        decode() again.
        Sample code:

        decoder = Decoder()
        for msg in msgs:
            decoder.decode(msg.encoding, msg.width, msg.height, msg.time_base, msg.events)
            cd_events = decoder.get_cd_events()
            trig_events = decoder.get_ext_trig_events()
)pbdoc")
    .def("decode", &Decoder::decode, R"pbdoc(
        decode(event_packet) -> None

        Decode message

        :param event_packet: event packet
        :type event_packet:  event_camera_msgs/msgs/EventPacket

)pbdoc")
    .def(pybind11::init<>())
    .def("decode_bytes", &Decoder::decode_bytes, R"pbdoc(
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
    .def("decode_array", &Decoder::decode_array, R"pbdoc(
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
    .def("decode_until", &Decoder::decode_until, R"pbdoc(
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
    .def("get_cd_events", &Decoder::get_cd_events, R"pbdoc(
        get_cd_events() -> numpy.ndarray['EventCD']

        Fetches decoded change detected (CD) events. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.

        :return: array of detected events in the same format as the metavision SDK uses.
        :rtype: numpy.ndarray[EventCD], structured numpy array with fields 'x', 'y', 't', 'p'
)pbdoc")
    .def("get_ext_trig_events", &Decoder::get_ext_trig_events, R"pbdoc(
        get_ext_trig_events() -> numpy.ndarray['EventExtTrig']

        Fetches decoded external trigger events. Will clear out decoded events, to be
        called only once. If not called, events will be lost next time decode() is called.
        :return: array of trigger events in the same format as the metavision SDK uses.
        :rtype: numpy.ndarray[EventExtTrig], structured numpy array with fields 'p', 't', 'id'
)pbdoc")
    .def("get_num_cd_on", &Decoder::get_num_cd_on, R"pbdoc(
        get_num_cd_on() -> int
        :return: cumulative number of ON events.
        :rtype: int
)pbdoc")
    .def("get_num_cd_off", &Decoder::get_num_cd_off, R"pbdoc(
        get_num_cd_off() -> int
        :return: cumulative number of OFF events.
        :rtype: int
)pbdoc")
    .def("get_num_trigger_rising", &Decoder::get_num_trigger_rising, R"pbdoc(
        get_num_trigger_rising() -> int
        :return: cumulative number of rising edge external trigger events.
        :rtype: int
)pbdoc")
    .def("get_num_trigger_falling", &Decoder::get_num_trigger_falling, R"pbdoc(
        get_num_trigger_falling() -> int
        :return: cumulative number of falling edge external trigger events.
        :rtype: int
)pbdoc");
}
