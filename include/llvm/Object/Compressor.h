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
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ELFTypes.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/Compression.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/Errc.h"

namespace llvm {
namespace object {

/// Compressor helps to handle compression of compressed sections.
class Compressor {
public:
  explicit Compressor(StringRef Data) : SectionData(Data) {}

  /// Compress section data.
  /// @param W Destination buffer stream for compressed data.
  Error writeCompressedSectionData(support::endian::Writer &W) {
    SmallVector<char, 128> CompressedBuffer;
    auto E = zlib::compress(SectionData, CompressedBuffer);
    W.OS << StringRef(CompressedBuffer.data(), CompressedBuffer.size()).str();
    return E;
  }

private:
  StringRef SectionData;
};

/// Returns a new debug section name based on Name and CompressionType.  This
/// function assumes that Name is one of the standard debug section names that
/// begins with either .debug or .zdebug: any other prefix will result in an
/// Error.
///
/// If CompressionType is GNU, then the name prefix is set to ".zdebug",
/// otherwise the prefix will be set to ".debug". This function can be used to
/// fixup GNU ".zdebug" names to the corresponding ".debug" name during
/// decompression.
Expected<std::string>
getNewDebugSectionName(const StringRef Name,
                       DebugCompressionType CompressionType);

/// Returns if the section can be compressed based on its name (must have a
/// debug name, starts with .*debug.
bool isCompressableSectionName(StringRef Name);

template <typename T> void writeReserved(support::endian::Writer &W) {
  if (T::Is64Bits)
    W.write(static_cast<ELF::Elf64_Word>(0)); // ch_reserved field.
}

/// Writes the proper compression header info to W depending on CompressionType.
template <typename T>
Error produceZLibHeader(support::endian::Writer &W, uint64_t DecompressedSize,
                        unsigned Align, DebugCompressionType CompressionType) {
  if (CompressionType != DebugCompressionType::GNU &&
      CompressionType != DebugCompressionType::Z) {
    return createStringError(
        llvm::errc::invalid_argument,
        "Invalid DebugCompressionType, only GNU and ZLIB are supported.");
  }

  if (CompressionType == DebugCompressionType::GNU) {
    const StringRef Magic = "ZLIB";
    W.OS << Magic;
    support::endian::write(W.OS, DecompressedSize, support::big);
    return Error::success();
  }

  using Chdr = Elf_Chdr_Impl<T>;
  W.write(static_cast<decltype(Chdr::ch_type)>(ELF::ELFCOMPRESS_ZLIB));
  writeReserved<T>(W);
  W.write(static_cast<decltype(Chdr::ch_size)>(DecompressedSize));
  W.write(static_cast<decltype(Chdr::ch_addralign)>(Align));
  return Error::success();
}

/// Returns compressed section content, including header data.
///
/// @param Contents        Section Content to be compressed.
/// @param Align           Alignment (used for Elf_Chdr).
/// @param CompressionType DebugCompressionType GNU or Z  (used for Elf_Chdr).
///
/// Return value is a tuple that includes if the zlib library hit any errors,
/// followed by a boolean denoting if the compressed content plus the header
/// length is smaller than before, and lastly followed by the actual compressed
/// section content.
template <typename T>
Expected<std::vector<char>> compress(StringRef Contents, uint64_t Align,
                                     DebugCompressionType CompressionType) {
  SmallVector<char, 128> CompressedContents;
  raw_svector_ostream OS(CompressedContents);
  support::endian::Writer W(OS, T::TargetEndianness);
  if (Error E =
          produceZLibHeader<T>(W, Contents.size(), Align, CompressionType))
    return std::move(E);
  Compressor C(Contents);
  if (Error E = C.writeCompressedSectionData(W))
    return std::move(E);
  return std::vector<char>(CompressedContents.begin(),
                           CompressedContents.end());
}

} // end namespace object
} // end namespace llvm

#endif // LLVM_OBJECT_COMPRESSOR_H
