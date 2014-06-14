// Copyright 2014 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
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

#include "src/ast-value-factory.h"

#include "src/api.h"
#include "src/objects.h"

namespace v8 {
namespace internal {

namespace {

template <typename Char>
int vector_hash(Vector<const Char> string) {
  int hash = 0;
  for (int i = 0; i < string.length(); i++) {
    int c = static_cast<int>(string[i]);
    hash += c;
    hash += (hash << 10);
    hash ^= (hash >> 6);
  }
  return hash;
}


// For using StringToArrayIndex.
class OneByteStringStream {
 public:
  explicit OneByteStringStream(Vector<const byte> lb) :
      literal_bytes_(lb), pos_(0) {}

  bool HasMore() { return pos_ < literal_bytes_.length(); }
  uint16_t GetNext() { return literal_bytes_[pos_++]; }

 private:
  Vector<const byte> literal_bytes_;
  int pos_;
};

}


bool AstString::AsArrayIndex(uint32_t* index) const {
  if (!string_.is_null())
    return string_->AsArrayIndex(index);
  if (!is_one_byte_ || literal_bytes_.length() == 0 ||
      literal_bytes_.length() > String::kMaxArrayIndexSize)
    return false;
  OneByteStringStream stream(literal_bytes_);
  return StringToArrayIndex(&stream, index);
}


bool AstString::IsOneByteEqualTo(const char* data) const {
  int length = static_cast<int>(strlen(data));
  if (is_one_byte_ && literal_bytes_.length() == length) {
    const char* token = reinterpret_cast<const char*>(literal_bytes_.start());
    return !strncmp(token, data, length);
  }
  return false;
}


void AstString::Internalize(Isolate* isolate) {
  if (!string_.is_null()) return;
  if (literal_bytes_.length() == 0) {
    string_ = isolate->factory()->empty_string();
  } else if (is_one_byte_) {
    string_ = isolate->factory()->InternalizeOneByteString(literal_bytes_);
  } else {
    string_ = isolate->factory()->InternalizeTwoByteString(
        Vector<const uint16_t>::cast(literal_bytes_));
  }
}


bool AstString::Compare(void* a, void* b) {
  AstString* string1 = reinterpret_cast<AstString*>(a);
  AstString* string2 = reinterpret_cast<AstString*>(b);
  if (string1->is_one_byte_ != string2->is_one_byte_) return false;
  if (string1->hash_ != string2->hash_) return false;
  int length = string1->literal_bytes_.length();
  if (string2->literal_bytes_.length() != length) return false;
  return memcmp(string1->literal_bytes_.start(),
                string2->literal_bytes_.start(), length) == 0;
}


bool AstValue::IsPropertyName() const {
  if (type_ == STRING) {
    uint32_t index;
    return !string_->AsArrayIndex(&index);
  }
  return false;
}


bool AstValue::BooleanValue() const {
  switch (type_) {
    case STRING:
      ASSERT(string_ != NULL);
      return !string_->IsEmpty();
    case SYMBOL:
      UNREACHABLE();
      break;
    case NUMBER:
      return DoubleToBoolean(number_);
    case SMI:
      return smi_ != 0;
    case STRING_ARRAY:
      UNREACHABLE();
      break;
    case BOOLEAN:
      return bool_;
    case NULL_TYPE:
      return false;
    case THE_HOLE:
      UNREACHABLE();
      break;
    case UNDEFINED:
      return false;
  }
  UNREACHABLE();
  return false;
}


void AstValue::Internalize(Isolate* isolate) {
  switch (type_) {
    case STRING:
      ASSERT(string_ != NULL);
      // Strings are already internalized.
      ASSERT(!string_->string().is_null());
      break;
    case SYMBOL:
      value_ = Object::GetProperty(
                   isolate, handle(isolate->native_context()->builtins()),
                   symbol_name_).ToHandleChecked();
      break;
    case NUMBER:
      value_ = isolate->factory()->NewNumber(number_, TENURED);
      break;
    case SMI:
      value_ = handle(Smi::FromInt(smi_), isolate);
      break;
    case BOOLEAN:
      if (bool_) {
        value_ = isolate->factory()->true_value();
      } else {
        value_ = isolate->factory()->false_value();
      }
      break;
    case STRING_ARRAY: {
      ASSERT(strings_ != NULL);
      Factory* factory = isolate->factory();
      int len = strings_->length();
      Handle<FixedArray> elements = factory->NewFixedArray(len, TENURED);
      for (int i = 0; i < len; i++) {
        const AstString* string = (*strings_)[i];
        Handle<Object> element = string->string();
        // Strings are already internalized.
        ASSERT(!element.is_null());
        elements->set(i, *element);
      }
      value_ =
          factory->NewJSArrayWithElements(elements, FAST_ELEMENTS, TENURED);
      break;
    }
    case NULL_TYPE:
      value_ = isolate->factory()->null_value();
      break;
    case THE_HOLE:
      value_ = isolate->factory()->the_hole_value();
      break;
    case UNDEFINED:
      value_ = isolate->factory()->undefined_value();
      break;
  }
}


const AstString* AstValueFactory::GetOneByteString(
    Vector<const uint8_t> literal) {
  return GetString(vector_hash(literal), true, literal);
}


const AstString* AstValueFactory::GetTwoByteString(
    Vector<const uint16_t> literal) {
  return GetString(vector_hash(literal), false,
                   Vector<const byte>::cast(literal));
}


const AstString* AstValueFactory::GetString(Handle<String> literal) {
  DisallowHeapAllocation no_gc;
  String::FlatContent content = literal->GetFlatContent();
  if (content.IsAscii()) {
    return GetOneByteString(content.ToOneByteVector());
  }
  ASSERT(content.IsTwoByte());
  return GetTwoByteString(content.ToUC16Vector());
}


void AstValueFactory::Internalize(Isolate* isolate) {
  if (isolate_) {
    // Everything is already internalized.
    return;
  }
  // Strings need to be internalized before values, because values refer to
  // strings.
  for (HashMap::Entry* p = string_table_.Start(); p != NULL;
       p = string_table_.Next(p)) {
    AstString* string = reinterpret_cast<AstString*>(p->key);
    string->Internalize(isolate);
  }
  for (int i = 0; i < values_.length(); ++i) {
    values_[i]->Internalize(isolate);
  }
  isolate_ = isolate;
}


const AstValue* AstValueFactory::NewString(const AstString* string) {
  AstValue* value = new (zone_) AstValue(string);
  ASSERT(string != NULL);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewSymbol(const char* name) {
  AstValue* value = new (zone_) AstValue(name);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewNumber(double number) {
  AstValue* value = new (zone_) AstValue(number);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewSmi(int number) {
  AstValue* value =
      new (zone_) AstValue(AstValue::SMI, number);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewBoolean(bool b) {
  AstValue* value = new (zone_) AstValue(b);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewStringList(
    ZoneList<const AstString*>* strings) {
  AstValue* value = new (zone_) AstValue(strings);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewNull() {
  AstValue* value = new (zone_) AstValue(AstValue::NULL_TYPE);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewUndefined() {
  AstValue* value = new (zone_) AstValue(AstValue::UNDEFINED);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstValue* AstValueFactory::NewTheHole() {
  AstValue* value = new (zone_) AstValue(AstValue::THE_HOLE);
  if (isolate_) {
    value->Internalize(isolate_);
  }
  values_.Add(value);
  return value;
}


const AstString* AstValueFactory::GetString(int hash, bool is_one_byte,
                                            Vector<const byte> literal_bytes) {
  // literal_bytes here points to whatever the user passed, and this is OK
  // because we use vector_compare (which checks the contents) to compare
  // against the AstStrings which are in the string_table_. We should not return
  // this AstString.
  AstString key(is_one_byte, literal_bytes, hash);
  HashMap::Entry* entry = string_table_.Lookup(&key, hash, true);
  if (entry->value == NULL) {
    // Copy literal contents for later comparison.
    key.literal_bytes_ =
        Vector<const byte>::cast(literal_chars_.AddBlock(literal_bytes));
    // This Vector will be valid as long as the Collector is alive (meaning that
    // the AstString will not be moved).
    Vector<AstString> new_string = string_table_keys_.AddBlock(1, key);
    entry->key = &new_string[0];
    if (isolate_) {
      new_string[0].Internalize(isolate_);
    }
    entry->value = reinterpret_cast<void*>(1);
  }
  return reinterpret_cast<AstString*>(entry->key);
}


} }  // namespace v8::internal