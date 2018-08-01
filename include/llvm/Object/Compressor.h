//===-- Compressor.h --------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===-----------------------------------------------------------------------===/

#ifndef LLVM_OBJECT_COMPRESSOR_H
#define LLVM_OBJECT_COMPRESSOR_H

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ELFTypes.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Compression.h"
#include "llvm/Support/EndianStream.h"

namespace llvm {
namespace object {

/// Return the gnu style compressed section name.
StringRef getDebugSectionName(const StringRef Name, bool IsGnuStyle);

/// Returns if the section can be compressed based on its name (must have a
/// debug name, starts with .*debug.
bool isCompressable(StringRef Name);

/// Returns if the section is already compressed based on the section contents
/// Name and Flags.
bool isCompressed(StringRef Name, uint64_t Flags);

template <typename T> void writeReserved(support::endian::Writer &W) {
  if (T::Is64Bits)
    W.write(static_cast<ELF::Elf64_Word>(0)); // ch_reserved field.
}

template <typename T>
void produceCompressedZLibHeader(support::endian::Writer &W,
                                 uint64_t DecompressedSize, unsigned Align) {
  using Chdr = Elf_Chdr_Impl<T>;
  W.write(static_cast<decltype(Chdr::ch_type)>(ELF::ELFCOMPRESS_ZLIB));
  writeReserved<T>(W);
  W.write(static_cast<decltype(Chdr::ch_size)>(DecompressedSize));
  W.write(static_cast<decltype(Chdr::ch_addralign)>(Align));
}

/// Compressor helps to handle compression of compressed sections.
template <typename T> class Compressor {
public:
  void writeHeader(support::endian::Writer &W, uint64_t DecompressedSize,
                   unsigned Alignment, bool IsGnuStyle) {
    if (IsGnuStyle) {
      const StringRef Magic = "ZLIB";
      W.OS << Magic;
      support::endian::write(W.OS, DecompressedSize, support::big);
      return;
    }

    produceCompressedZLibHeader<T>(W, DecompressedSize, Alignment);
  }

  /// Uncompress section data to raw buffer provided.
  /// @param W        Destination buffer stream.
  Error writeCompressedSectionData(support::endian::Writer &W) {
    SmallVector<char, 128> CompressedBuffer;
    auto E = zlib::compress(SectionData, CompressedBuffer);
    W.OS << StringRef(CompressedBuffer.data(), CompressedBuffer.size()).str();
    return E;
  }

  explicit Compressor(StringRef Data) : SectionData(Data) {}

private:
  StringRef SectionData;
};

} // end namespace object
} // end namespace llvm

#endif // LLVM_OBJECT_COMPRESSOR_H
