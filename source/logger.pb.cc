// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: logger.proto

#include "logger.pb.h"

#include <algorithm>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/wire_format_lite.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)
#include <google/protobuf/port_def.inc>
namespace logger {
class LogRequestDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<LogRequest> _instance;
} _LogRequest_default_instance_;
class LogReplyDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<LogReply> _instance;
} _LogReply_default_instance_;
class MinLevelRequestDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<MinLevelRequest> _instance;
} _MinLevelRequest_default_instance_;
class MinLevelReplyDefaultTypeInternal {
 public:
  ::PROTOBUF_NAMESPACE_ID::internal::ExplicitlyConstructed<MinLevelReply> _instance;
} _MinLevelReply_default_instance_;
}  // namespace logger
static void InitDefaultsscc_info_LogReply_logger_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::logger::_LogReply_default_instance_;
    new (ptr) ::logger::LogReply();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
  ::logger::LogReply::InitAsDefaultInstance();
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_LogReply_logger_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 0, 0, InitDefaultsscc_info_LogReply_logger_2eproto}, {}};

static void InitDefaultsscc_info_LogRequest_logger_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::logger::_LogRequest_default_instance_;
    new (ptr) ::logger::LogRequest();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
  ::logger::LogRequest::InitAsDefaultInstance();
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_LogRequest_logger_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 0, 0, InitDefaultsscc_info_LogRequest_logger_2eproto}, {}};

static void InitDefaultsscc_info_MinLevelReply_logger_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::logger::_MinLevelReply_default_instance_;
    new (ptr) ::logger::MinLevelReply();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
  ::logger::MinLevelReply::InitAsDefaultInstance();
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_MinLevelReply_logger_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 0, 0, InitDefaultsscc_info_MinLevelReply_logger_2eproto}, {}};

static void InitDefaultsscc_info_MinLevelRequest_logger_2eproto() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  {
    void* ptr = &::logger::_MinLevelRequest_default_instance_;
    new (ptr) ::logger::MinLevelRequest();
    ::PROTOBUF_NAMESPACE_ID::internal::OnShutdownDestroyMessage(ptr);
  }
  ::logger::MinLevelRequest::InitAsDefaultInstance();
}

::PROTOBUF_NAMESPACE_ID::internal::SCCInfo<0> scc_info_MinLevelRequest_logger_2eproto =
    {{ATOMIC_VAR_INIT(::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase::kUninitialized), 0, 0, InitDefaultsscc_info_MinLevelRequest_logger_2eproto}, {}};

static ::PROTOBUF_NAMESPACE_ID::Metadata file_level_metadata_logger_2eproto[4];
static constexpr ::PROTOBUF_NAMESPACE_ID::EnumDescriptor const** file_level_enum_descriptors_logger_2eproto = nullptr;
static constexpr ::PROTOBUF_NAMESPACE_ID::ServiceDescriptor const** file_level_service_descriptors_logger_2eproto = nullptr;

const ::PROTOBUF_NAMESPACE_ID::uint32 TableStruct_logger_2eproto::offsets[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::logger::LogRequest, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::logger::LogRequest, level_),
  PROTOBUF_FIELD_OFFSET(::logger::LogRequest, module_name_),
  PROTOBUF_FIELD_OFFSET(::logger::LogRequest, message_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::logger::LogReply, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::logger::LogReply, status_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::logger::MinLevelRequest, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::logger::MinLevelRequest, level_),
  ~0u,  // no _has_bits_
  PROTOBUF_FIELD_OFFSET(::logger::MinLevelReply, _internal_metadata_),
  ~0u,  // no _extensions_
  ~0u,  // no _oneof_case_
  ~0u,  // no _weak_field_map_
  PROTOBUF_FIELD_OFFSET(::logger::MinLevelReply, status_),
};
static const ::PROTOBUF_NAMESPACE_ID::internal::MigrationSchema schemas[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) = {
  { 0, -1, sizeof(::logger::LogRequest)},
  { 8, -1, sizeof(::logger::LogReply)},
  { 14, -1, sizeof(::logger::MinLevelRequest)},
  { 20, -1, sizeof(::logger::MinLevelReply)},
};

static ::PROTOBUF_NAMESPACE_ID::Message const * const file_default_instances[] = {
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::logger::_LogRequest_default_instance_),
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::logger::_LogReply_default_instance_),
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::logger::_MinLevelRequest_default_instance_),
  reinterpret_cast<const ::PROTOBUF_NAMESPACE_ID::Message*>(&::logger::_MinLevelReply_default_instance_),
};

const char descriptor_table_protodef_logger_2eproto[] PROTOBUF_SECTION_VARIABLE(protodesc_cold) =
  "\n\014logger.proto\022\006logger\"A\n\nLogRequest\022\r\n\005"
  "level\030\001 \001(\t\022\023\n\013module_name\030\002 \001(\t\022\017\n\007mess"
  "age\030\003 \001(\t\"\032\n\010LogReply\022\016\n\006status\030\001 \001(\005\" \n"
  "\017MinLevelRequest\022\r\n\005level\030\001 \001(\t\"\037\n\rMinLe"
  "velReply\022\016\n\006status\030\001 \001(\0052:\n\006Logger\0220\n\006Ad"
  "dLog\022\022.logger.LogRequest\032\020.logger.LogRep"
  "ly\"\000b\006proto3"
  ;
static const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable*const descriptor_table_logger_2eproto_deps[1] = {
};
static ::PROTOBUF_NAMESPACE_ID::internal::SCCInfoBase*const descriptor_table_logger_2eproto_sccs[4] = {
  &scc_info_LogReply_logger_2eproto.base,
  &scc_info_LogRequest_logger_2eproto.base,
  &scc_info_MinLevelReply_logger_2eproto.base,
  &scc_info_MinLevelRequest_logger_2eproto.base,
};
static ::PROTOBUF_NAMESPACE_ID::internal::once_flag descriptor_table_logger_2eproto_once;
static bool descriptor_table_logger_2eproto_initialized = false;
const ::PROTOBUF_NAMESPACE_ID::internal::DescriptorTable descriptor_table_logger_2eproto = {
  &descriptor_table_logger_2eproto_initialized, descriptor_table_protodef_logger_2eproto, "logger.proto", 252,
  &descriptor_table_logger_2eproto_once, descriptor_table_logger_2eproto_sccs, descriptor_table_logger_2eproto_deps, 4, 0,
  schemas, file_default_instances, TableStruct_logger_2eproto::offsets,
  file_level_metadata_logger_2eproto, 4, file_level_enum_descriptors_logger_2eproto, file_level_service_descriptors_logger_2eproto,
};

// Force running AddDescriptors() at dynamic initialization time.
static bool dynamic_init_dummy_logger_2eproto = (  ::PROTOBUF_NAMESPACE_ID::internal::AddDescriptors(&descriptor_table_logger_2eproto), true);
namespace logger {

// ===================================================================

void LogRequest::InitAsDefaultInstance() {
}
class LogRequest::_Internal {
 public:
};

LogRequest::LogRequest()
  : ::PROTOBUF_NAMESPACE_ID::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:logger.LogRequest)
}
LogRequest::LogRequest(const LogRequest& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      _internal_metadata_(nullptr) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  level_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  if (!from._internal_level().empty()) {
    level_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.level_);
  }
  module_name_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  if (!from._internal_module_name().empty()) {
    module_name_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.module_name_);
  }
  message_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  if (!from._internal_message().empty()) {
    message_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.message_);
  }
  // @@protoc_insertion_point(copy_constructor:logger.LogRequest)
}

void LogRequest::SharedCtor() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&scc_info_LogRequest_logger_2eproto.base);
  level_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  module_name_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  message_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

LogRequest::~LogRequest() {
  // @@protoc_insertion_point(destructor:logger.LogRequest)
  SharedDtor();
}

void LogRequest::SharedDtor() {
  level_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  module_name_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  message_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

void LogRequest::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const LogRequest& LogRequest::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_LogRequest_logger_2eproto.base);
  return *internal_default_instance();
}


void LogRequest::Clear() {
// @@protoc_insertion_point(message_clear_start:logger.LogRequest)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  level_.ClearToEmptyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  module_name_.ClearToEmptyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  message_.ClearToEmptyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  _internal_metadata_.Clear();
}

const char* LogRequest::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // string level = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 10)) {
          auto str = _internal_mutable_level();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(::PROTOBUF_NAMESPACE_ID::internal::VerifyUTF8(str, "logger.LogRequest.level"));
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // string module_name = 2;
      case 2:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 18)) {
          auto str = _internal_mutable_module_name();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(::PROTOBUF_NAMESPACE_ID::internal::VerifyUTF8(str, "logger.LogRequest.module_name"));
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      // string message = 3;
      case 3:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 26)) {
          auto str = _internal_mutable_message();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(::PROTOBUF_NAMESPACE_ID::internal::VerifyUTF8(str, "logger.LogRequest.message"));
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag, &_internal_metadata_, ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* LogRequest::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:logger.LogRequest)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string level = 1;
  if (this->level().size() > 0) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_level().data(), static_cast<int>(this->_internal_level().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "logger.LogRequest.level");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_level(), target);
  }

  // string module_name = 2;
  if (this->module_name().size() > 0) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_module_name().data(), static_cast<int>(this->_internal_module_name().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "logger.LogRequest.module_name");
    target = stream->WriteStringMaybeAliased(
        2, this->_internal_module_name(), target);
  }

  // string message = 3;
  if (this->message().size() > 0) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_message().data(), static_cast<int>(this->_internal_message().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "logger.LogRequest.message");
    target = stream->WriteStringMaybeAliased(
        3, this->_internal_message(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:logger.LogRequest)
  return target;
}

size_t LogRequest::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:logger.LogRequest)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string level = 1;
  if (this->level().size() > 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_level());
  }

  // string module_name = 2;
  if (this->module_name().size() > 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_module_name());
  }

  // string message = 3;
  if (this->message().size() > 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_message());
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void LogRequest::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:logger.LogRequest)
  GOOGLE_DCHECK_NE(&from, this);
  const LogRequest* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<LogRequest>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:logger.LogRequest)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:logger.LogRequest)
    MergeFrom(*source);
  }
}

void LogRequest::MergeFrom(const LogRequest& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:logger.LogRequest)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.level().size() > 0) {

    level_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.level_);
  }
  if (from.module_name().size() > 0) {

    module_name_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.module_name_);
  }
  if (from.message().size() > 0) {

    message_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.message_);
  }
}

void LogRequest::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:logger.LogRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void LogRequest::CopyFrom(const LogRequest& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:logger.LogRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LogRequest::IsInitialized() const {
  return true;
}

void LogRequest::InternalSwap(LogRequest* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  level_.Swap(&other->level_, &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  module_name_.Swap(&other->module_name_, &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
  message_.Swap(&other->message_, &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
}

::PROTOBUF_NAMESPACE_ID::Metadata LogRequest::GetMetadata() const {
  return GetMetadataStatic();
}


// ===================================================================

void LogReply::InitAsDefaultInstance() {
}
class LogReply::_Internal {
 public:
};

LogReply::LogReply()
  : ::PROTOBUF_NAMESPACE_ID::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:logger.LogReply)
}
LogReply::LogReply(const LogReply& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      _internal_metadata_(nullptr) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  status_ = from.status_;
  // @@protoc_insertion_point(copy_constructor:logger.LogReply)
}

void LogReply::SharedCtor() {
  status_ = 0;
}

LogReply::~LogReply() {
  // @@protoc_insertion_point(destructor:logger.LogReply)
  SharedDtor();
}

void LogReply::SharedDtor() {
}

void LogReply::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const LogReply& LogReply::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_LogReply_logger_2eproto.base);
  return *internal_default_instance();
}


void LogReply::Clear() {
// @@protoc_insertion_point(message_clear_start:logger.LogReply)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  status_ = 0;
  _internal_metadata_.Clear();
}

const char* LogReply::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // int32 status = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 8)) {
          status_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag, &_internal_metadata_, ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* LogReply::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:logger.LogReply)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // int32 status = 1;
  if (this->status() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(1, this->_internal_status(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:logger.LogReply)
  return target;
}

size_t LogReply::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:logger.LogReply)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // int32 status = 1;
  if (this->status() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_status());
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void LogReply::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:logger.LogReply)
  GOOGLE_DCHECK_NE(&from, this);
  const LogReply* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<LogReply>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:logger.LogReply)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:logger.LogReply)
    MergeFrom(*source);
  }
}

void LogReply::MergeFrom(const LogReply& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:logger.LogReply)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.status() != 0) {
    _internal_set_status(from._internal_status());
  }
}

void LogReply::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:logger.LogReply)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void LogReply::CopyFrom(const LogReply& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:logger.LogReply)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool LogReply::IsInitialized() const {
  return true;
}

void LogReply::InternalSwap(LogReply* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  swap(status_, other->status_);
}

::PROTOBUF_NAMESPACE_ID::Metadata LogReply::GetMetadata() const {
  return GetMetadataStatic();
}


// ===================================================================

void MinLevelRequest::InitAsDefaultInstance() {
}
class MinLevelRequest::_Internal {
 public:
};

MinLevelRequest::MinLevelRequest()
  : ::PROTOBUF_NAMESPACE_ID::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:logger.MinLevelRequest)
}
MinLevelRequest::MinLevelRequest(const MinLevelRequest& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      _internal_metadata_(nullptr) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  level_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  if (!from._internal_level().empty()) {
    level_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.level_);
  }
  // @@protoc_insertion_point(copy_constructor:logger.MinLevelRequest)
}

void MinLevelRequest::SharedCtor() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&scc_info_MinLevelRequest_logger_2eproto.base);
  level_.UnsafeSetDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

MinLevelRequest::~MinLevelRequest() {
  // @@protoc_insertion_point(destructor:logger.MinLevelRequest)
  SharedDtor();
}

void MinLevelRequest::SharedDtor() {
  level_.DestroyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
}

void MinLevelRequest::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const MinLevelRequest& MinLevelRequest::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_MinLevelRequest_logger_2eproto.base);
  return *internal_default_instance();
}


void MinLevelRequest::Clear() {
// @@protoc_insertion_point(message_clear_start:logger.MinLevelRequest)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  level_.ClearToEmptyNoArena(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited());
  _internal_metadata_.Clear();
}

const char* MinLevelRequest::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // string level = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 10)) {
          auto str = _internal_mutable_level();
          ptr = ::PROTOBUF_NAMESPACE_ID::internal::InlineGreedyStringParser(str, ptr, ctx);
          CHK_(::PROTOBUF_NAMESPACE_ID::internal::VerifyUTF8(str, "logger.MinLevelRequest.level"));
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag, &_internal_metadata_, ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* MinLevelRequest::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:logger.MinLevelRequest)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // string level = 1;
  if (this->level().size() > 0) {
    ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::VerifyUtf8String(
      this->_internal_level().data(), static_cast<int>(this->_internal_level().length()),
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::SERIALIZE,
      "logger.MinLevelRequest.level");
    target = stream->WriteStringMaybeAliased(
        1, this->_internal_level(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:logger.MinLevelRequest)
  return target;
}

size_t MinLevelRequest::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:logger.MinLevelRequest)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // string level = 1;
  if (this->level().size() > 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::StringSize(
        this->_internal_level());
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void MinLevelRequest::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:logger.MinLevelRequest)
  GOOGLE_DCHECK_NE(&from, this);
  const MinLevelRequest* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<MinLevelRequest>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:logger.MinLevelRequest)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:logger.MinLevelRequest)
    MergeFrom(*source);
  }
}

void MinLevelRequest::MergeFrom(const MinLevelRequest& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:logger.MinLevelRequest)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.level().size() > 0) {

    level_.AssignWithDefault(&::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(), from.level_);
  }
}

void MinLevelRequest::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:logger.MinLevelRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void MinLevelRequest::CopyFrom(const MinLevelRequest& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:logger.MinLevelRequest)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool MinLevelRequest::IsInitialized() const {
  return true;
}

void MinLevelRequest::InternalSwap(MinLevelRequest* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  level_.Swap(&other->level_, &::PROTOBUF_NAMESPACE_ID::internal::GetEmptyStringAlreadyInited(),
    GetArenaNoVirtual());
}

::PROTOBUF_NAMESPACE_ID::Metadata MinLevelRequest::GetMetadata() const {
  return GetMetadataStatic();
}


// ===================================================================

void MinLevelReply::InitAsDefaultInstance() {
}
class MinLevelReply::_Internal {
 public:
};

MinLevelReply::MinLevelReply()
  : ::PROTOBUF_NAMESPACE_ID::Message(), _internal_metadata_(nullptr) {
  SharedCtor();
  // @@protoc_insertion_point(constructor:logger.MinLevelReply)
}
MinLevelReply::MinLevelReply(const MinLevelReply& from)
  : ::PROTOBUF_NAMESPACE_ID::Message(),
      _internal_metadata_(nullptr) {
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  status_ = from.status_;
  // @@protoc_insertion_point(copy_constructor:logger.MinLevelReply)
}

void MinLevelReply::SharedCtor() {
  status_ = 0;
}

MinLevelReply::~MinLevelReply() {
  // @@protoc_insertion_point(destructor:logger.MinLevelReply)
  SharedDtor();
}

void MinLevelReply::SharedDtor() {
}

void MinLevelReply::SetCachedSize(int size) const {
  _cached_size_.Set(size);
}
const MinLevelReply& MinLevelReply::default_instance() {
  ::PROTOBUF_NAMESPACE_ID::internal::InitSCC(&::scc_info_MinLevelReply_logger_2eproto.base);
  return *internal_default_instance();
}


void MinLevelReply::Clear() {
// @@protoc_insertion_point(message_clear_start:logger.MinLevelReply)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  status_ = 0;
  _internal_metadata_.Clear();
}

const char* MinLevelReply::_InternalParse(const char* ptr, ::PROTOBUF_NAMESPACE_ID::internal::ParseContext* ctx) {
#define CHK_(x) if (PROTOBUF_PREDICT_FALSE(!(x))) goto failure
  while (!ctx->Done(&ptr)) {
    ::PROTOBUF_NAMESPACE_ID::uint32 tag;
    ptr = ::PROTOBUF_NAMESPACE_ID::internal::ReadTag(ptr, &tag);
    CHK_(ptr);
    switch (tag >> 3) {
      // int32 status = 1;
      case 1:
        if (PROTOBUF_PREDICT_TRUE(static_cast<::PROTOBUF_NAMESPACE_ID::uint8>(tag) == 8)) {
          status_ = ::PROTOBUF_NAMESPACE_ID::internal::ReadVarint(&ptr);
          CHK_(ptr);
        } else goto handle_unusual;
        continue;
      default: {
      handle_unusual:
        if ((tag & 7) == 4 || tag == 0) {
          ctx->SetLastTag(tag);
          goto success;
        }
        ptr = UnknownFieldParse(tag, &_internal_metadata_, ptr, ctx);
        CHK_(ptr != nullptr);
        continue;
      }
    }  // switch
  }  // while
success:
  return ptr;
failure:
  ptr = nullptr;
  goto success;
#undef CHK_
}

::PROTOBUF_NAMESPACE_ID::uint8* MinLevelReply::_InternalSerialize(
    ::PROTOBUF_NAMESPACE_ID::uint8* target, ::PROTOBUF_NAMESPACE_ID::io::EpsCopyOutputStream* stream) const {
  // @@protoc_insertion_point(serialize_to_array_start:logger.MinLevelReply)
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  // int32 status = 1;
  if (this->status() != 0) {
    target = stream->EnsureSpace(target);
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::WriteInt32ToArray(1, this->_internal_status(), target);
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    target = ::PROTOBUF_NAMESPACE_ID::internal::WireFormat::InternalSerializeUnknownFieldsToArray(
        _internal_metadata_.unknown_fields(), target, stream);
  }
  // @@protoc_insertion_point(serialize_to_array_end:logger.MinLevelReply)
  return target;
}

size_t MinLevelReply::ByteSizeLong() const {
// @@protoc_insertion_point(message_byte_size_start:logger.MinLevelReply)
  size_t total_size = 0;

  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  // Prevent compiler warnings about cached_has_bits being unused
  (void) cached_has_bits;

  // int32 status = 1;
  if (this->status() != 0) {
    total_size += 1 +
      ::PROTOBUF_NAMESPACE_ID::internal::WireFormatLite::Int32Size(
        this->_internal_status());
  }

  if (PROTOBUF_PREDICT_FALSE(_internal_metadata_.have_unknown_fields())) {
    return ::PROTOBUF_NAMESPACE_ID::internal::ComputeUnknownFieldsSize(
        _internal_metadata_, total_size, &_cached_size_);
  }
  int cached_size = ::PROTOBUF_NAMESPACE_ID::internal::ToCachedSize(total_size);
  SetCachedSize(cached_size);
  return total_size;
}

void MinLevelReply::MergeFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_merge_from_start:logger.MinLevelReply)
  GOOGLE_DCHECK_NE(&from, this);
  const MinLevelReply* source =
      ::PROTOBUF_NAMESPACE_ID::DynamicCastToGenerated<MinLevelReply>(
          &from);
  if (source == nullptr) {
  // @@protoc_insertion_point(generalized_merge_from_cast_fail:logger.MinLevelReply)
    ::PROTOBUF_NAMESPACE_ID::internal::ReflectionOps::Merge(from, this);
  } else {
  // @@protoc_insertion_point(generalized_merge_from_cast_success:logger.MinLevelReply)
    MergeFrom(*source);
  }
}

void MinLevelReply::MergeFrom(const MinLevelReply& from) {
// @@protoc_insertion_point(class_specific_merge_from_start:logger.MinLevelReply)
  GOOGLE_DCHECK_NE(&from, this);
  _internal_metadata_.MergeFrom(from._internal_metadata_);
  ::PROTOBUF_NAMESPACE_ID::uint32 cached_has_bits = 0;
  (void) cached_has_bits;

  if (from.status() != 0) {
    _internal_set_status(from._internal_status());
  }
}

void MinLevelReply::CopyFrom(const ::PROTOBUF_NAMESPACE_ID::Message& from) {
// @@protoc_insertion_point(generalized_copy_from_start:logger.MinLevelReply)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void MinLevelReply::CopyFrom(const MinLevelReply& from) {
// @@protoc_insertion_point(class_specific_copy_from_start:logger.MinLevelReply)
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool MinLevelReply::IsInitialized() const {
  return true;
}

void MinLevelReply::InternalSwap(MinLevelReply* other) {
  using std::swap;
  _internal_metadata_.Swap(&other->_internal_metadata_);
  swap(status_, other->status_);
}

::PROTOBUF_NAMESPACE_ID::Metadata MinLevelReply::GetMetadata() const {
  return GetMetadataStatic();
}


// @@protoc_insertion_point(namespace_scope)
}  // namespace logger
PROTOBUF_NAMESPACE_OPEN
template<> PROTOBUF_NOINLINE ::logger::LogRequest* Arena::CreateMaybeMessage< ::logger::LogRequest >(Arena* arena) {
  return Arena::CreateInternal< ::logger::LogRequest >(arena);
}
template<> PROTOBUF_NOINLINE ::logger::LogReply* Arena::CreateMaybeMessage< ::logger::LogReply >(Arena* arena) {
  return Arena::CreateInternal< ::logger::LogReply >(arena);
}
template<> PROTOBUF_NOINLINE ::logger::MinLevelRequest* Arena::CreateMaybeMessage< ::logger::MinLevelRequest >(Arena* arena) {
  return Arena::CreateInternal< ::logger::MinLevelRequest >(arena);
}
template<> PROTOBUF_NOINLINE ::logger::MinLevelReply* Arena::CreateMaybeMessage< ::logger::MinLevelReply >(Arena* arena) {
  return Arena::CreateInternal< ::logger::MinLevelReply >(arena);
}
PROTOBUF_NAMESPACE_CLOSE

// @@protoc_insertion_point(global_scope)
#include <google/protobuf/port_undef.inc>