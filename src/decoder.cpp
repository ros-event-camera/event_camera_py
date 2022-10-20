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

#include <event_array_codecs/decoder.h>
#include <event_array_py/decoder.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <cstdint>
#include <iostream>

Decoder::Decoder() {}

void Decoder::decode(const std::string & encoding, uint64_t timeBase, pybind11::bytes events)
{
  auto decoder = decoderFactory_.getInstance(encoding);
  if (decoder) {
    decoder->setTimeBase(timeBase);
    decoder->setTimeMultiplier(1);  // report in usecs instead of nanoseconds
    const size_t bufSize = PyByteArray_GET_SIZE(events.ptr());
    // for some reason this returns a bad pointer
    // const uint8_t *buf = reinterpret_cast<uint8_t *>(PyByteArray_AsString(events.ptr()));
    // TODO(Bernd): avoid the memory copy here
    const std::string foo(events);  // creates unnecessary copy!
    delete cdEvents_;               // in case events have not been picked up
    cdEvents_ = new std::vector<EventCD>();
    // TODO(Bernd): use hack here to avoid initializing the memory
    cdEvents_->reserve(maxSizeCD_);
    const uint8_t * buf = reinterpret_cast<const uint8_t *>(&foo[0]);
    decoder->decode(buf, bufSize, this);
  }
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

void Decoder::eventCD(uint64_t sensor_time, uint16_t ex, uint16_t ey, uint8_t polarity)
{
  cdEvents_->push_back(EventCD(ex, ey, polarity, sensor_time / 1000));
  maxSizeCD_ = std::max(cdEvents_->size(), maxSizeCD_);
  numCDEvents_[std::min(polarity, uint8_t(1))]++;
}

void Decoder::eventExtTrigger(uint64_t sensor_time, uint8_t edge, uint8_t id)
{
  (void)sensor_time;
  (void)edge;
  (void)id;
  numTriggerEvents_[std::min(edge, uint8_t(1))]++;
}

PYBIND11_MODULE(event_array_decoder, m)
{
  PYBIND11_NUMPY_DTYPE(EventCD, x, y, p, t);

  pybind11::class_<Decoder>(m, "Decoder")
    .def(pybind11::init<>())
    .def("decode", &Decoder::decode, "Decode event array message")
    .def("get_cd_events", &Decoder::get_cd_events, "Get decoded CD events")
    .def("get_num_cd_on", &Decoder::get_num_cd_on, "Get number of ON events")
    .def("get_num_cd_off", &Decoder::get_num_cd_off, "Get number of OFF events")
    .def(
      "get_num_trigger_rising", &Decoder::get_num_trigger_rising,
      "Get number of rising edge trigger events")
    .def(
      "get_num_trigger_falling", &Decoder::get_num_trigger_falling,
      "Get number of falling edge trigger events");
}
