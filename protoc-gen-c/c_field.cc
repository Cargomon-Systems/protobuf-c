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
#include <google/protobuf/stubs/common.h>

#include <protobuf-c/protobuf-c.pb.h>

#include "c_bytes_field.h"
#include "c_enum_field.h"
#include "c_field.h"
#include "c_helpers.h"
#include "c_message_field.h"
#include "c_primitive_field.h"
#include "c_string_field.h"

namespace protobuf_c {

FieldGenerator::~FieldGenerator()
{
}

static bool is_packable_type(google::protobuf::FieldDescriptor::Type type)
{
  return type == google::protobuf::FieldDescriptor::TYPE_DOUBLE
      || type == google::protobuf::FieldDescriptor::TYPE_FLOAT
      || type == google::protobuf::FieldDescriptor::TYPE_INT64
      || type == google::protobuf::FieldDescriptor::TYPE_UINT64
      || type == google::protobuf::FieldDescriptor::TYPE_INT32
      || type == google::protobuf::FieldDescriptor::TYPE_FIXED64
      || type == google::protobuf::FieldDescriptor::TYPE_FIXED32
      || type == google::protobuf::FieldDescriptor::TYPE_BOOL
      || type == google::protobuf::FieldDescriptor::TYPE_UINT32
      || type == google::protobuf::FieldDescriptor::TYPE_ENUM
      || type == google::protobuf::FieldDescriptor::TYPE_SFIXED32
      || type == google::protobuf::FieldDescriptor::TYPE_SFIXED64
      || type == google::protobuf::FieldDescriptor::TYPE_SINT32
      || type == google::protobuf::FieldDescriptor::TYPE_SINT64;
    //TYPE_BYTES
    //TYPE_STRING
    //TYPE_GROUP
    //TYPE_MESSAGE
}

void FieldGenerator::GenerateDescriptorInitializerGeneric(google::protobuf::io::Printer* printer,
							  bool optional_uses_has,
							  const std::string &type_macro,
							  const std::string &descriptor_addr) const
{
  std::map<std::string, std::string> variables;
  const google::protobuf::OneofDescriptor *oneof = descriptor_->containing_oneof();
  const ProtobufCFileOptions opt = descriptor_->file()->options().GetExtension(pb_c_file);
  variables["TYPE"] = type_macro;
  variables["classname"] = FullNameToC(FieldScope(descriptor_)->full_name(), FieldScope(descriptor_)->file());
  variables["name"] = FieldName(descriptor_);
  if (opt.use_oneof_field_name())
    variables["proto_name"] = oneof->name();
  else
    variables["proto_name"] = descriptor_->name();
  variables["descriptor_addr"] = descriptor_addr;
  variables["value"] = SimpleItoa(descriptor_->number());
  if (oneof != NULL)
    variables["oneofname"] = CamelToLower(oneof->name());

  if (FieldSyntax(descriptor_) == 3 &&
    descriptor_->label() == google::protobuf::FieldDescriptor::LABEL_OPTIONAL) {
    variables["LABEL"] = "NONE";
    optional_uses_has = false;
  } else {
    variables["LABEL"] = CamelToUpper(GetLabelName(descriptor_->label()));
  }

  if (descriptor_->has_default_value()) {
    variables["default_value"] = std::string("&")
                               + FullNameToLower(descriptor_->full_name(), descriptor_->file())
			       + "__default_value";
  } else if (FieldSyntax(descriptor_) == 3 &&
    descriptor_->type() == google::protobuf::FieldDescriptor::TYPE_STRING) {
    variables["default_value"] = "&protobuf_c_empty_string";
  } else {
    variables["default_value"] = "NULL";
  }

  variables["flags"] = "0";

  if (descriptor_->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED
   && is_packable_type (descriptor_->type())
   && descriptor_->options().packed()) {
    variables["flags"] += " | PROTOBUF_C_FIELD_FLAG_PACKED";
  } else if (descriptor_->label() == google::protobuf::FieldDescriptor::LABEL_REPEATED
   && is_packable_type (descriptor_->type())
   && FieldSyntax(descriptor_) == 3
   && !descriptor_->options().has_packed()) {
    variables["flags"] += " | PROTOBUF_C_FIELD_FLAG_PACKED";
  }

  if (descriptor_->options().deprecated())
    variables["flags"] += " | PROTOBUF_C_FIELD_FLAG_DEPRECATED";

  if (oneof != NULL)
    variables["flags"] += " | PROTOBUF_C_FIELD_FLAG_ONEOF";

  // Eliminate codesmell "or with 0"
  if (variables["flags"].find("0 | ") == 0) {
   variables["flags"].erase(0, 4);
  }

  printer->Print("{\n");
  if (descriptor_->file()->options().has_optimize_for() &&
        descriptor_->file()->options().optimize_for() ==
        google::protobuf::FileOptions_OptimizeMode_CODE_SIZE) {
     printer->Print("  NULL, /* CODE_SIZE */\n");
  } else {
     printer->Print(variables, "  \"$proto_name$\",\n");
  }
  printer->Print(variables,
    "  $value$,\n"
    "  PROTOBUF_C_LABEL_$LABEL$,\n"
    "  PROTOBUF_C_TYPE_$TYPE$,\n");
  switch (descriptor_->label()) {
    case google::protobuf::FieldDescriptor::LABEL_REQUIRED:
      printer->Print(variables, "  0,   /* quantifier_offset */\n");
      break;
    case google::protobuf::FieldDescriptor::LABEL_OPTIONAL:
      if (oneof != NULL) {
        printer->Print(variables, "  offsetof($classname$, $oneofname$_case),\n");
      } else if (optional_uses_has) {
	printer->Print(variables, "  offsetof($classname$, has_$name$),\n");
      } else {
	printer->Print(variables, "  0,   /* quantifier_offset */\n");
      }
      break;
    case google::protobuf::FieldDescriptor::LABEL_REPEATED:
      printer->Print(variables, "  offsetof($classname$, n_$name$),\n");
      break;
  }
  printer->Print(variables, "  offsetof($classname$, $name$),\n");
  printer->Print(variables, "  $descriptor_addr$,\n");
  printer->Print(variables, "  $default_value$,\n");
  printer->Print(variables, "  $flags$,             /* flags */\n");
  printer->Print(variables, "  0,NULL,NULL    /* reserved1,reserved2, etc */\n");
  printer->Print("},\n");
}

FieldGeneratorMap::FieldGeneratorMap(const google::protobuf::Descriptor* descriptor)
  : descriptor_(descriptor),
    field_generators_(
      new std::unique_ptr<FieldGenerator>[descriptor->field_count()]) {
  // Construct all the FieldGenerators.
  for (int i = 0; i < descriptor->field_count(); i++) {
    field_generators_[i].reset(MakeGenerator(descriptor->field(i)));
  }
}

FieldGenerator* FieldGeneratorMap::MakeGenerator(const google::protobuf::FieldDescriptor* field) {
  const ProtobufCFieldOptions opt = field->options().GetExtension(pb_c_field);
  switch (field->type()) {
    case google::protobuf::FieldDescriptor::TYPE_MESSAGE:
      return new MessageFieldGenerator(field);
    case google::protobuf::FieldDescriptor::TYPE_STRING:
      if (opt.string_as_bytes())
        return new BytesFieldGenerator(field);
      else
        return new StringFieldGenerator(field);
    case google::protobuf::FieldDescriptor::TYPE_BYTES:
      return new BytesFieldGenerator(field);
    case google::protobuf::FieldDescriptor::TYPE_ENUM:
      return new EnumFieldGenerator(field);
    case google::protobuf::FieldDescriptor::TYPE_GROUP:
      return 0;			// XXX
    default:
      return new PrimitiveFieldGenerator(field);
  }
}

FieldGeneratorMap::~FieldGeneratorMap() {}

const FieldGenerator& FieldGeneratorMap::get(
    const google::protobuf::FieldDescriptor* field) const {
  GOOGLE_CHECK_EQ(field->containing_type(), descriptor_);
  return *field_generators_[field->index()];
}

}  // namespace protobuf_c
