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

#ifndef EVENT_CAMERA_PY__DECODER_H_
#define EVENT_CAMERA_PY__DECODER_H_

#include <event_camera_codecs/decoder.h>
#include <event_camera_codecs/decoder_factory.h>
#include <event_camera_codecs/event_packet.h>
#include <event_camera_py/event_cd.h>
#include <event_camera_py/event_ext_trig.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <string>
#include <tuple>
#include <variant>
#include <vector>

template <class A>
class Decoder
{
public:
  Decoder() = default;
  void decode(pybind11::object msg)
  {
    pybind11::object eventsObj = get_attr<pybind11::object>(msg, "events");
    Py_buffer view;
    if (PyObject_GetBuffer(eventsObj.ptr(), &view, PyBUF_CONTIG_RO) != 0) {
      throw std::runtime_error("cannot convert events to byte buffer");
    }

    do_full_decode(
      get_attr<std::string>(msg, "encoding"), get_attr<uint32_t>(msg, "width"),
      get_attr<uint32_t>(msg, "height"), get_attr<uint64_t>(msg, "time_base"),
      reinterpret_cast<const uint8_t *>(view.buf), view.len);
    PyBuffer_Release(&view);
  }

  std::tuple<bool, uint64_t> decode_until(pybind11::object msg, uint64_t untilTime)
  {
    auto decoder = initialize_decoder(
      get_attr<std::string>(msg, "encoding"), get_attr<uint32_t>(msg, "width"),
      get_attr<uint32_t>(msg, "height"));
    accumulator_.setHasSensorTimeSinceEpoch(decoder->hasSensorTimeSinceEpoch());
    accumulator_.reset_stored_events();
    decoder->setTimeBase(get_attr<uint64_t>(msg, "time_base"));
    pybind11::object eventsObj = get_attr<pybind11::object>(msg, "events");
    Py_buffer view;
    if (PyObject_GetBuffer(eventsObj.ptr(), &view, PyBUF_CONTIG_RO) != 0) {
      throw std::runtime_error("cannot convert events to byte buffer");
    }
    uint64_t nextTime{0};
    const bool reachedTimeLimit = decoder->decodeUntil(
      reinterpret_cast<const uint8_t *>(view.buf), view.len, &accumulator_, untilTime,
      get_attr<uint64_t>(msg, "time_base"), &nextTime);
    PyBuffer_Release(&view);
    return (std::tuple<bool, uint64_t>({reachedTimeLimit, nextTime}));
  }

  std::variant<uint64_t, pybind11::none> find_first_sensor_time(pybind11::object msg)
  {
    auto decoder = initialize_decoder(
      get_attr<std::string>(msg, "encoding"), get_attr<uint32_t>(msg, "width"),
      get_attr<uint32_t>(msg, "height"));

    pybind11::object eventsObj = get_attr<pybind11::object>(msg, "events");
    Py_buffer view;
    if (PyObject_GetBuffer(eventsObj.ptr(), &view, PyBUF_CONTIG_RO) != 0) {
      throw std::runtime_error("cannot convert events to byte buffer");
    }
    decoder->setTimeBase(get_attr<uint64_t>(msg, "time_base"));
    uint64_t firstTime{0};
    const bool foundTime = decoder->findFirstSensorTime(
      reinterpret_cast<const uint8_t *>(view.buf), view.len, &firstTime);
    PyBuffer_Release(&view);
    if (foundTime) {
      return (firstTime);
    } else {
      return (pybind11::cast<pybind11::none>(Py_None));
    }
  }

  void decode_bytes(
    const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
    pybind11::bytes events)
  {
    const uint8_t * buf = reinterpret_cast<const uint8_t *>(PyBytes_AsString(events.ptr()));
    do_full_decode(encoding, width, height, timeBase, buf, PyBytes_Size(events.ptr()));
  }

  void decode_array(
    const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
    pybind11::array_t<uint8_t> events)
  {
    if (events.ndim() != 1 || !pybind11::isinstance<pybind11::array_t<uint8_t>>(events)) {
      throw std::runtime_error("Input events must be 1-D numpy array of type uint8");
    }
    const uint8_t * buf = reinterpret_cast<const uint8_t *>(events.data());
    do_full_decode(encoding, width, height, timeBase, buf, events.size());
  }
  uint64_t get_start_time() const { return (accumulator_.get_start_time()); }
  pybind11::array_t<EventCD> get_cd_events() { return (accumulator_.get_cd_events()); }
  pybind11::array_t<EventExtTrig> get_ext_trig_events()
  {
    return (accumulator_.get_ext_trig_events());
  };
  pybind11::list get_cd_event_packets() { return (accumulator_.get_cd_event_packets()); }
  pybind11::list get_ext_trig_event_packets()
  {
    return (accumulator_.get_ext_trig_event_packets());
  }

  size_t get_num_cd_off() const { return (accumulator_.get_num_cd_off()); }
  size_t get_num_cd_on() const { return (accumulator_.get_num_cd_on()); }
  size_t get_num_trigger_rising() const { return (accumulator_.get_num_trigger_rising()); }
  size_t get_num_trigger_falling() const { return (accumulator_.get_num_trigger_falling()); }

private:
  using DecoderType = event_camera_codecs::Decoder<event_camera_codecs::EventPacket, A>;
  template <class T>
  static T get_attr(pybind11::object msg, const char * name)
  {
    return (pybind11::getattr(msg, name)).cast<T>();
  }

  void do_full_decode(
    const std::string & encoding, uint16_t width, uint16_t height, uint64_t timeBase,
    const uint8_t * buf, size_t bufSize)
  {
    auto decoder = initialize_decoder(encoding, width, height);
    decoder->setTimeBase(timeBase);
    accumulator_.setHasSensorTimeSinceEpoch(decoder->hasSensorTimeSinceEpoch());
    accumulator_.reset_stored_events();
    decoder->decode(buf, bufSize, &accumulator_);
  }

  DecoderType * initialize_decoder(const std::string & encoding, uint32_t width, uint32_t height)
  {
    accumulator_.initialize(width, height);
    // this will only create a decoder on the first call, subsequently return instance
    auto decoder = decoderFactory_.getInstance(encoding, width, height);
    if (!decoder) {
      throw(std::runtime_error("no decoder for encoding " + encoding));
    }
    decoder->setTimeMultiplier(1);  // report in usecs instead of nanoseconds
    return (decoder);
  }

  // ------------ variables
  event_camera_codecs::DecoderFactory<event_camera_codecs::EventPacket, A> decoderFactory_;
  A accumulator_;
};

#endif  // EVENT_CAMERA_PY__DECODER_H_
