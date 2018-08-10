//===-- Compressor.cpp ----------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Object/Compressor.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Support/Compression.h"
#include "llvm/Support/DataExtractor.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/Errc.h"

using namespace llvm;
using namespace llvm::support::endian;
using namespace object;

namespace llvm {
namespace object {

bool isCompressableSectionName(StringRef Name) {
  return (Name.startswith(".debug") || Name.startswith(".zdebug"));
}

Expected<std::string>
getNewDebugSectionName(const StringRef Name,
                       DebugCompressionType CompressionType) {
  if (!isCompressableSectionName(Name)) {
    return createStringError(llvm::errc::invalid_argument,
                             "Invalid Debug Section Name: %s.",
                             Name.str().c_str());
  }
  std::string Prefix =
      (CompressionType == DebugCompressionType::GNU) ? ".z" : ".";
  return Prefix + Name.substr(Name.startswith(".debug") ? 1 : 2).str();
}

} // end namespace object
} // end namespace llvm
