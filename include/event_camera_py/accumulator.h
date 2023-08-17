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

#ifndef EVENT_CAMERA_PY__ACCUMULATOR_H_
#define EVENT_CAMERA_PY__ACCUMULATOR_H_

#include <event_camera_codecs/decoder.h>
#include <event_camera_codecs/decoder_factory.h>
#include <event_camera_codecs/event_packet.h>
#include <event_camera_py/event_cd.h>
#include <event_camera_py/event_ext_trig.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#include <string>
#include <tuple>
#include <vector>

class Accumulator : public event_camera_codecs::EventProcessor
{
public:
  // inherited from EventProcessor
  void eventCD(uint64_t sensor_time, uint16_t ex, uint16_t ey, uint8_t polarity) override
  {
    cdEvents_->push_back(EventCD(ex, ey, polarity, sensor_time));
    maxSizeCD_ = std::max(cdEvents_->size(), maxSizeCD_);
    numCDEvents_[std::min(polarity, uint8_t(1))]++;
  }

  void eventExtTrigger(uint64_t sensor_time, uint8_t edge, uint8_t id) override
  {
    extTrigEvents_->push_back(EventExtTrig(
      static_cast<int16_t>(edge), static_cast<int64_t>(sensor_time), static_cast<int16_t>(id)));
    maxSizeExtTrig_ = std::max(extTrigEvents_->size(), maxSizeExtTrig_);
    numExtTrigEvents_[std::min(edge, uint8_t(1))]++;
  }

  void finished() override {}
  void rawData(const char *, size_t) override {}

  // own methods
  void reset_stored_events()
  {
    delete cdEvents_;  // in case events have not been picked up
    cdEvents_ = new std::vector<EventCD>();
    delete extTrigEvents_;  // in case events have not been picked up
    extTrigEvents_ = new std::vector<EventExtTrig>();
    // TODO(Bernd): use hack here to avoid initializing the memory
    cdEvents_->reserve(maxSizeCD_);
    extTrigEvents_->reserve(maxSizeExtTrig_);
  }

  pybind11::array_t<EventCD> get_cd_events()
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
  pybind11::array_t<EventExtTrig> get_ext_trig_events()
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

  pybind11::list get_cd_event_packets() { return (pybind11::list()); }
  pybind11::list get_ext_trig_event_packets() { return (pybind11::list()); }

  size_t get_num_cd_off() const { return (numCDEvents_[0]); }
  size_t get_num_cd_on() const { return (numCDEvents_[1]); }
  size_t get_num_trigger_rising() const { return (numExtTrigEvents_[0]); }
  size_t get_num_trigger_falling() const { return (numExtTrigEvents_[1]); }
  void initialize(uint32_t, uint32_t) {}

private:
  // ------------ variables
  size_t numCDEvents_[2] = {0, 0};
  size_t numExtTrigEvents_[2] = {0, 0};
  std::vector<EventCD> * cdEvents_{0};
  std::vector<EventExtTrig> * extTrigEvents_{0};
  size_t maxSizeCD_{0};
  size_t maxSizeExtTrig_{0};
};

#endif  // EVENT_CAMERA_PY__ACCUMULATOR_H_
