// Copyright 2011 The Native Client SDK Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "examples/graphics/life/life.h"

#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/var.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace {
const char* const kUpdateMethodId = "update";
const char* const kAddCellAtPointMethodId = "addCellAtPoint";
const unsigned int kInitialRandSeed = 0xC0DE533D;

inline uint32_t MakeRGBA(uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
  return (((a) << 24) | ((r) << 16) | ((g) << 8) | (b));
}

// Map of neighboring colors.
const uint32_t kNeighborColors[] = {
    MakeRGBA(0x00, 0x00, 0x00, 0xff),
    MakeRGBA(0x00, 0x40, 0x00, 0xff),
    MakeRGBA(0x00, 0x60, 0x00, 0xff),
    MakeRGBA(0x00, 0x80, 0x00, 0xff),
    MakeRGBA(0x00, 0xA0, 0x00, 0xff),
    MakeRGBA(0x00, 0xC0, 0x00, 0xff),
    MakeRGBA(0x00, 0xE0, 0x00, 0xff),
    MakeRGBA(0x00, 0x00, 0x00, 0xff),
    MakeRGBA(0x00, 0x40, 0x00, 0xff),
    MakeRGBA(0x00, 0x60, 0x00, 0xff),
    MakeRGBA(0x00, 0x80, 0x00, 0xff),
    MakeRGBA(0x00, 0xA0, 0x00, 0xff),
    MakeRGBA(0x00, 0xC0, 0x00, 0xff),
    MakeRGBA(0x00, 0xE0, 0x00, 0xff),
    MakeRGBA(0x00, 0xFF, 0x00, 0xff),
    MakeRGBA(0x00, 0xFF, 0x00, 0xff),
    MakeRGBA(0x00, 0xFF, 0x00, 0xff),
    MakeRGBA(0x00, 0xFF, 0x00, 0xff),
};

// These represent the new health value of a cell based on its neighboring
// values.  The health is binary: either alive or dead.
const uint8_t kIsAlive[] = {
      0, 0, 0, 1, 0, 0, 0, 0, 0,  // Values if the center cell is dead.
      0, 0, 1, 1, 0, 0, 0, 0, 0  // Values if the center cell is alive.
  };

void FlushCallback(void* data, int32_t result) {
  static_cast<life::Life*>(data)->set_flush_pending(false);
}
}  // namespace

namespace life {
Life::Life(PP_Instance instance) : pp::Instance(instance),
                                   graphics_2d_context_(NULL),
                                   pixel_buffer_(NULL),
                                   random_bits_(kInitialRandSeed),
                                   flush_pending_(false),
                                   cell_in_(NULL),
                                   cell_out_(NULL) {
}

Life::~Life() {
  delete[] cell_in_;
  delete[] cell_out_;
  DestroyContext();
  delete pixel_buffer_;
}

pp::Var Life::GetInstanceObject() {
  LifeScriptObject* script_object = new LifeScriptObject(this);
  return pp::Var(this, script_object);
}

bool Life::Init(uint32_t argc, const char* argn[], const char* argv[]) {
  return true;
}

void Life::Plot(int x, int y) {
  if (cell_in_ == NULL)
    return;
  if (x < 0) return;
  if (x >= width()) return;
  if (y < 0) return;
  if (y >= height()) return;
  *(cell_in_ + x + y * width()) = 1;
}

void Life::DidChangeView(const pp::Rect& position, const pp::Rect& clip) {
  if (position.size().width() == width() &&
      position.size().height() == height())
    return;  // Size didn't change, no need to update anything.

  // Create a new device context with the new size.
  DestroyContext();
  CreateContext(position.size());
  // Delete the old pixel buffer and create a new one.
  delete pixel_buffer_;
  delete[] cell_in_;
  delete[] cell_out_;
  pixel_buffer_ = NULL;
  cell_in_ = cell_out_ = NULL;
  if (graphics_2d_context_ != NULL) {
    pixel_buffer_ = new pp::ImageData(this,
                                      PP_IMAGEDATAFORMAT_BGRA_PREMUL,
                                      graphics_2d_context_->size(),
                                      false);
    const size_t size = width() * height();
    cell_in_ = new uint8_t[size];
    cell_out_ = new uint8_t[size];
    std::fill(cell_in_, cell_in_ + size, 0);
    std::fill(cell_out_, cell_out_ + size, 0);
  }
}

void Life::Update() {
  Stir();
  UpdateCells();
  Swap();
  FlushPixelBuffer();
}

void Life::AddCellAtPoint(const pp::Var& var_x, const pp::Var& var_y) {
  if (!var_x.is_number() || !var_y.is_number())
    return;
  int32_t x, y;
  x = var_x.is_int() ? var_x.AsInt() : static_cast<int32_t>(var_x.AsDouble());
  y = var_y.is_int() ? var_y.AsInt() : static_cast<int32_t>(var_y.AsDouble());
  Plot(x - 1, y - 1);
  Plot(x + 0, y - 1);
  Plot(x + 1, y - 1);
  Plot(x - 1, y + 0);
  Plot(x + 0, y + 0);
  Plot(x + 1, y + 0);
  Plot(x - 1, y + 1);
  Plot(x + 0, y + 1);
  Plot(x + 1, y + 1);
}

void Life::Stir() {
  if (cell_in_ == NULL || cell_out_ == NULL)
    return;
  const int height = this->height();
  const int width = this->width();
  for (int i = 0; i < width; ++i) {
    cell_in_[i] = random_bits_.value();
    cell_in_[i + (height - 1) * width] = random_bits_.value();
  }
  for (int i = 0; i < height; ++i) {
    cell_in_[i * width] = random_bits_.value();
    cell_in_[i * width + (width - 1)] = random_bits_.value();
  }
}

void Life::UpdateCells() {
  if (cell_in_ == NULL || cell_out_ == NULL || pixels() == NULL)
    return;
  const int height = this->height();
  const int width = this->width();
  // Do neighbor sumation; apply rules, output pixel color.
  for (int y = 1; y < (height - 1); ++y) {
    uint8_t *src0 = (cell_in_ + (y - 1) * width) + 1;
    uint8_t *src1 = src0 + width;
    uint8_t *src2 = src1 + width;
    int count;
    uint32_t color;
    uint8_t *dst = (cell_out_ + (y) * width) + 1;
    uint32_t *pixel_buffer = pixels() + y * width;
    for (int x = 1; x < (width - 1); ++x) {
      // Build sum, weight center by 9x.
      count = src0[-1] + src0[0] +     src0[1] +
              src1[-1] + src1[0] * 9 + src1[1] +
              src2[-1] + src2[0] +     src2[1];
      color = kNeighborColors[count];
      *pixel_buffer++ = color;
      *dst++ = kIsAlive[count];
      ++src0;
      ++src1;
      ++src2;
    }
  }
}

void Life::Swap() {
  uint8_t* tmp = cell_in_;
  cell_in_ = cell_out_;
  cell_out_ = tmp;
}

void Life::CreateContext(const pp::Size& size) {
  if (IsContextValid())
    return;
  graphics_2d_context_ = new pp::Graphics2D(this, size, false);
  if (!BindGraphics(*graphics_2d_context_)) {
    printf("Couldn't bind the device context\n");
  }
}

void Life::DestroyContext() {
  if (!IsContextValid())
    return;
  delete graphics_2d_context_;
  graphics_2d_context_ = NULL;
}

void Life::FlushPixelBuffer() {
  if (!IsContextValid())
    return;
  graphics_2d_context_->PaintImageData(*pixel_buffer_, pp::Point());
  if (flush_pending())
    return;
  set_flush_pending(true);
  graphics_2d_context_->Flush(pp::CompletionCallback(&FlushCallback, this));
}

bool Life::LifeScriptObject::HasMethod(
    const pp::Var& method,
    pp::Var* exception) {
  if (!method.is_string()) {
    return false;
  }
  std::string method_name = method.AsString();
  return method_name == kUpdateMethodId ||
         method_name == kAddCellAtPointMethodId;
}

pp::Var Life::LifeScriptObject::Call(
    const pp::Var& method,
    const std::vector<pp::Var>& args,
    pp::Var* exception) {
  if (!method.is_string()) {
    return pp::Var(false);
  }
  std::string method_name = method.AsString();
  if (app_instance_ != NULL) {
    if (method_name == kUpdateMethodId) {
      app_instance_->Update();
    } else if (method_name == kAddCellAtPointMethodId) {
      // Pull off the first two params.
      if (args.size() < 2)
        return pp::Var(false);
      app_instance_->AddCellAtPoint(args[0], args[1]);
    }
  }
  return pp::Var();
}

uint8_t Life::RandomBitGenerator::value() {
  return static_cast<uint8_t>(rand_r(&random_bit_seed_) & 1);
}

}  // namespace life

