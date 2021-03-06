/*
    RawSpeed - RAW file decoder.

    Copyright (C) 2018 Stefan Löffler

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#pragma once

#include "common/Common.h"                      // for uint32
#include "common/RawImage.h"                    // for RawImageData
#include "decompressors/AbstractDecompressor.h" // for AbstractDecompressor
#include "io/BitPumpMSB.h"                      // for BitPumpMSB
#include "io/ByteStream.h"                      // for ByteStream

namespace rawspeed {

const int MAX_NUM_CHANNELS = 4;
const int MAX_NUM_WAVELETS = 3;
const int MAX_NUM_BANDS = 4;
const int MAX_NUM_SUBBANDS = 10;
const int MAX_NUM_PRESCALE = 8;

class ByteStream;
class RawImage;

// Decompresses VC-5 as used by GoPro

#define VC5_LOG_TABLE_SIZE 4096

class VC5Decompressor final : public AbstractDecompressor {
  RawImage mImg;
  ByteStream mBs;
  unsigned int* mVC5LogTable;

  struct {
    ushort16 numChannels, numSubbands, numWavelets;
    ushort16 iChannel, iSubband;
    ushort16 imgWidth, imgHeight, imgFormat;
    ushort16 patternWidth, patternHeight;
    ushort16 cps, bpc, lowpassPrecision;
    uint8_t image_sequence_identifier[16];
    uint32_t image_sequence_number;
    short16 quantization;
  } mVC5;

  template <class T> class Array2D {
    unsigned int _pitch;
    T* _data;

  public:
    unsigned int width, height;

    Array2D();
    Array2D(T* data, const unsigned int dataWidth,
            const unsigned int dataHeight, const unsigned int dataPitch = 0);

    static Array2D<T> create(const unsigned int width,
                             const unsigned int height);
    void destroy();
    void clear();
    inline T& operator()(const unsigned int x, const unsigned int y);
    inline T operator()(const unsigned int x, const unsigned int y) const;
  };

  class Wavelet {
  public:
    uint16_t width, height, pitch;
    uint16_t numBands;
    uint16_t scale[MAX_NUM_BANDS];
    int16_t quant[MAX_NUM_BANDS];
    int16_t* data[MAX_NUM_BANDS];

    Wavelet();
    virtual ~Wavelet() { clear(); }

    void initialize(uint16_t waveletWidth, uint16_t waveletHeight);
    void clear();

    bool isInitialized() const { return mInitialized; }
    void setBandValid(const int band);
    bool isBandValid(const int band) const;
    uint32_t getValidBandMask() const { return mDecodedBandMask; }
    bool allBandsValid() const;

    void reconstructLowband(Array2D<int16_t> dest, const int16_t prescale,
                            const bool clampUint = false);

    Array2D<int16_t> bandAsArray2D(const unsigned int iBand);

  protected:
    uint32 mDecodedBandMask;
    bool mInitialized;

    static void dequantize(Array2D<int16_t> out, Array2D<int16_t> in,
                           int16_t quant);
  };
  struct Transform {
    Wavelet wavelet[MAX_NUM_WAVELETS];
    int16_t prescale[MAX_NUM_WAVELETS];
  } mTransforms[MAX_NUM_CHANNELS];

  static void getRLV(BitPumpMSB& bits, int& value, unsigned int& count);
  inline unsigned int DecodeLog(const int val) const;

public:
  VC5Decompressor(ByteStream bs, const RawImage& img);
  virtual ~VC5Decompressor();

  void decode(const unsigned int offsetX, const unsigned int offsetY);
};

} // namespace rawspeed
