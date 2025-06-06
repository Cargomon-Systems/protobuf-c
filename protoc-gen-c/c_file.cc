// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

// Copyright (c) 2008-2025, Dave Benson and the protobuf-c authors.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Modified to implement C code by Dave Benson.

#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/printer.h>

#include <protobuf-c/protobuf-c.pb.h>
#include "protobuf-c.h"

#include "c_enum.h"
#include "c_extension.h"
#include "c_file.h"
#include "c_helpers.h"
#include "c_message.h"
#include "c_service.h"

namespace protobuf_c {

// ===================================================================

FileGenerator::FileGenerator(const google::protobuf::FileDescriptor* file,
                             const std::string& dllexport_decl)
  : file_(file),
    message_generators_(
      new std::unique_ptr<MessageGenerator>[file->message_type_count()]),
    enum_generators_(
      new std::unique_ptr<EnumGenerator>[file->enum_type_count()]),
    service_generators_(
      new std::unique_ptr<ServiceGenerator>[file->service_count()]),
    extension_generators_(
      new std::unique_ptr<ExtensionGenerator>[file->extension_count()]) {

  for (int i = 0; i < file->message_type_count(); i++) {
    message_generators_[i].reset(
      new MessageGenerator(file->message_type(i), dllexport_decl));
  }

  for (int i = 0; i < file->enum_type_count(); i++) {
    enum_generators_[i].reset(
      new EnumGenerator(file->enum_type(i), dllexport_decl));
  }

  for (int i = 0; i < file->service_count(); i++) {
    service_generators_[i].reset(
      new ServiceGenerator(file->service(i), dllexport_decl));
  }

  for (int i = 0; i < file->extension_count(); i++) {
    extension_generators_[i].reset(
      new ExtensionGenerator(file->extension(i), dllexport_decl));
  }
}

FileGenerator::~FileGenerator() {}

void FileGenerator::GenerateHeader(google::protobuf::io::Printer* printer) {
  std::string filename_identifier = FilenameIdentifier(file_->name());

  const int min_header_version = 1003000;

  // Generate top of header.
  printer->Print(
    "/* Generated by the protocol buffer compiler.  DO NOT EDIT! */\n"
    "/* Generated from: $filename$ */\n"
    "\n"
    "#ifndef PROTOBUF_C_$filename_identifier$__INCLUDED\n"
    "#define PROTOBUF_C_$filename_identifier$__INCLUDED\n"
    "\n"
    "#include <protobuf-c/protobuf-c.h>\n"
    "\n"
    "PROTOBUF_C__BEGIN_DECLS\n"
    "\n",
    "filename", file_->name(),
    "filename_identifier", filename_identifier);

  // Verify the protobuf-c library header version is compatible with the
  // protoc-gen-c version before going any further.
  printer->Print(
    "#if PROTOBUF_C_VERSION_NUMBER < $min_header_version$\n"
    "# error This file was generated by a newer version of protobuf-c which is "
    "incompatible with your libprotobuf-c headers. Please update your headers.\n"
    "#elif $protoc_version$ < PROTOBUF_C_MIN_COMPILER_VERSION\n"
    "# error This file was generated by an older version of protobuf-c which is "
    "incompatible with your libprotobuf-c headers. Please regenerate this file "
    "with a newer version of protobuf-c.\n"
    "#endif\n"
    "\n",
    "min_header_version", SimpleItoa(min_header_version),
    "protoc_version", SimpleItoa(PROTOBUF_C_VERSION_NUMBER));

  for (int i = 0; i < file_->dependency_count(); i++) {
    const ProtobufCFileOptions opt =
	    file_->dependency(i)->options().GetExtension(pb_c_file);
    if (!opt.no_generate()) {
      printer->Print(
        "#include \"$dependency$.pb-c.h\"\n",
	"dependency", StripProto(file_->dependency(i)->name()));
    }
  }

  printer->Print("\n");

  // Generate forward declarations of classes.
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateStructTypedef(printer);
  }

  printer->Print("\n");

  // Generate enum definitions.
  printer->Print("\n/* --- enums --- */\n\n");
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateEnumDefinitions(printer);
  }
  for (int i = 0; i < file_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateDefinition(printer);
  }

  // Generate class definitions.
  printer->Print("\n/* --- messages --- */\n\n");
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateStructDefinition(printer);
  }

  for (int i = 0; i < file_->message_type_count(); i++) {
    const ProtobufCFileOptions opt = file_->options().GetExtension(pb_c_file);

    message_generators_[i]->GenerateHelperFunctionDeclarations(
						printer,
						opt.has_gen_pack_helpers(),
						opt.gen_pack_helpers(),
						opt.gen_init_helpers());
  }

  printer->Print("/* --- per-message closures --- */\n\n");
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateClosureTypedef(printer);
  }

  // Generate service definitions.
  printer->Print("\n/* --- services --- */\n\n");
  for (int i = 0; i < file_->service_count(); i++) {
    service_generators_[i]->GenerateMainHFile(printer);
  }

  // Declare extension identifiers.
  for (int i = 0; i < file_->extension_count(); i++) {
    extension_generators_[i]->GenerateDeclaration(printer);
  }

  printer->Print("\n/* --- descriptors --- */\n\n");
  for (int i = 0; i < file_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateDescriptorDeclarations(printer);
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateDescriptorDeclarations(printer);
  }
  for (int i = 0; i < file_->service_count(); i++) {
    service_generators_[i]->GenerateDescriptorDeclarations(printer);
  }

  printer->Print(
    "\n"
    "PROTOBUF_C__END_DECLS\n"
    "\n\n#endif  /* PROTOBUF_C_$filename_identifier$__INCLUDED */\n",
    "filename_identifier", filename_identifier);
}

void FileGenerator::GenerateSource(google::protobuf::io::Printer* printer) {
  printer->Print(
    "/* Generated by the protocol buffer compiler.  DO NOT EDIT! */\n"
    "/* Generated from: $filename$ */\n"
    "\n"
    "/* Do not generate deprecated warnings for self */\n"
    "#ifndef PROTOBUF_C__NO_DEPRECATED\n"
    "#define PROTOBUF_C__NO_DEPRECATED\n"
    "#endif\n"
    "\n"
    "#include \"$basename$.pb-c.h\"\n",
    "filename", file_->name(),
    "basename", StripProto(file_->name()));

  const ProtobufCFileOptions opt = file_->options().GetExtension(pb_c_file);

  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateHelperFunctionDefinitions(
						printer,
						opt.has_gen_pack_helpers(),
						opt.gen_pack_helpers(),
						opt.gen_init_helpers());
  }
  for (int i = 0; i < file_->message_type_count(); i++) {
    message_generators_[i]->GenerateMessageDescriptor(printer,
						      opt.gen_init_helpers());
  }
  for (int i = 0; i < file_->enum_type_count(); i++) {
    enum_generators_[i]->GenerateEnumDescriptor(printer);
  }
  for (int i = 0; i < file_->service_count(); i++) {
    service_generators_[i]->GenerateCFile(printer);
  }

}

}  // namespace protobuf_c
