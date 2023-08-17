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

#ifndef EVENT_CAMERA_PY__ACCUMULATOR_UNIQUE_H_
#define EVENT_CAMERA_PY__ACCUMULATOR_UNIQUE_H_

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

class AccumulatorUnique : public event_camera_codecs::EventProcessor
{
public:
  // inherited from EventProcessor
  void eventCD(uint64_t sensor_time, uint16_t ex, uint16_t ey, uint8_t polarity) override
  {
    if (pixelIsSet(ex, ey)) {
      // an event already happend at this location, clear the image and create
      // a new packet
      clearImage();
      setPixel(ex, ey);  // mark the pixel in the freshly cleaned image
      cdEvents_.push_back(new std::vector<EventCD>());
      cdEvents_.back()->reserve(maxSizeCD_);
    } else {
      setPixel(ex, ey);
    }
    if (cdEvents_.empty()) {
      cdEvents_.push_back(new std::vector<EventCD>());
    }
    cdEvents_.back()->push_back(EventCD(ex, ey, polarity, sensor_time));

    maxSizeCD_ = std::max(cdEvents_.back()->size(), maxSizeCD_);
    numCDEvents_[std::min(polarity, uint8_t(1))]++;
  }

  void eventExtTrigger(uint64_t sensor_time, uint8_t edge, uint8_t id) override
  {
    // It is not yet clear what a good policy would be for the external triggers,
    // so just pass all of them in a single packet.
    if (extTrigEvents_.empty()) {
      extTrigEvents_.push_back(new std::vector<EventExtTrig>());
      extTrigEvents_.back()->reserve(maxSizeExtTrig_);
    }
    extTrigEvents_.back()->push_back(EventExtTrig(
      static_cast<int16_t>(edge), static_cast<int64_t>(sensor_time), static_cast<int16_t>(id)));

    maxSizeExtTrig_ = std::max(extTrigEvents_.back()->size(), maxSizeExtTrig_);
    numExtTrigEvents_[std::min(edge, uint8_t(1))]++;
  }
  void finished() override {}
  void rawData(const char *, size_t) override {}

  void reset_stored_events()
  {
    // In case nobody has ever picked up the packets, delete them since
    // no python object holds a pointer to it.
    for (auto & p : cdEvents_) {
      delete p;
    }
    cdEvents_.clear();
    for (auto & p : extTrigEvents_) {
      delete p;
    }
    extTrigEvents_.clear();
  }

  pybind11::array_t<EventCD> get_cd_events() { return (pybind11::array_t<EventCD>()); }

  pybind11::array_t<EventExtTrig> get_ext_trig_events()
  {
    return (pybind11::array_t<EventExtTrig>());
  }

  template <class EventT>
  pybind11::list get_event_packets(std::vector<std::vector<EventT> *> * pkts)
  {
    pybind11::list packetList;
    for (auto & p : *pkts) {
      auto cap =
        pybind11::capsule(p, [](void * v) { delete reinterpret_cast<std::vector<EventT> *>(v); });
      packetList.append(pybind11::array_t<EventT>(p->size(), p->data(), cap));
      // now throw away the pointer without deleting the memory, since
      // a python object now will manage the memory
      p = nullptr;
    }
    pkts->clear();
    return (packetList);
  }

  bool pixelIsSet(uint16_t ex, uint16_t ey) const
  {
    const uint32_t offset = (ey * width_ + ex) / 8;
    return (image_[offset] & (1 << (ex % 8)));
  }

  void setPixel(uint16_t ex, uint16_t ey)
  {
    const uint32_t offset = (ey * width_ + ex) / 8;
    image_[offset] |= (1 << (ex % 8));
  }

  void initialize(uint32_t width, uint32_t height)
  {
    if (image_.empty()) {
      width_ = width;
      if (width == 0 || height == 0) {
        throw(std::runtime_error("bad sensor resolution width or height"));
      }
      image_.resize((width_ * height + 7) / 8, 0);
    }
  }

  void clearImage() { memset(image_.data(), 0, image_.size()); }

  pybind11::list get_cd_event_packets() { return (get_event_packets(&cdEvents_)); }
  pybind11::list get_ext_trig_event_packets() { return (get_event_packets(&extTrigEvents_)); }

  size_t get_num_cd_off() const { return (numCDEvents_[0]); }
  size_t get_num_cd_on() const { return (numCDEvents_[1]); }
  size_t get_num_trigger_rising() const { return (numExtTrigEvents_[0]); }
  size_t get_num_trigger_falling() const { return (numExtTrigEvents_[1]); }

private:
  // ------------ variables
  size_t numCDEvents_[2] = {0, 0};
  size_t numExtTrigEvents_[2] = {0, 0};
  std::vector<std::vector<EventCD> *> cdEvents_;
  std::vector<std::vector<EventExtTrig> *> extTrigEvents_;
  size_t maxSizeCD_{0};
  size_t maxSizeExtTrig_{0};
  std::vector<uint8_t> image_;
  uint32_t width_{0};
};

#endif  // EVENT_CAMERA_PY__ACCUMULATOR_UNIQUE_H_
