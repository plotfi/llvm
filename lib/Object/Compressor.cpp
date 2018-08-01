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

using namespace llvm;
using namespace llvm::support::endian;
using namespace object;

namespace llvm {
namespace object {

StringRef getDebugSectionName(const StringRef Name, bool IsGnuStyle) {

  if (!Name.startswith(".debug") && !Name.startswith(".zdebug"))
    return "";

  std::string LookupName = Name.substr(Name.startswith(".debug") ? 1 : 2).str();
  std::string Dot(".");

  static std::map<std::string, std::tuple<std::string, std::string>> NameMap;
  auto I = NameMap.find(LookupName);
  if (I == NameMap.end()) {
    NameMap.insert(std::pair<std::string, std::tuple<std::string, std::string>>(
        LookupName,
        std::make_tuple(Dot + LookupName, (Dot + "z") + LookupName)));
    I = NameMap.find(LookupName);
  }

  StringRef debugName, zdebugName;
  std::tie(debugName, zdebugName) = I->second;
  return IsGnuStyle ? zdebugName : debugName;
}

bool isCompressable(StringRef Name) {
  StringRef DotDebugName = getDebugSectionName(Name, false /* IsGnuStyle */);
  return DotDebugName.startswith(".debug");
}

bool isCompressed(StringRef Name, uint64_t Flags) {
  return Name.startswith(".zdebug") || (Flags & ELF::SHF_COMPRESSED);
}
} // end namespace object
} // end namespace llvm
