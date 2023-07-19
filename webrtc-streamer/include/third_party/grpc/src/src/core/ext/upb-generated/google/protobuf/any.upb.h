/* This file was generated by upbc (the upb compiler) from the input
 * file:
 *
 *     google/protobuf/any.proto
 *
 * Do not edit -- your changes will be discarded when the file is
 * regenerated. */

#ifndef GOOGLE_PROTOBUF_ANY_PROTO_UPB_H_
#define GOOGLE_PROTOBUF_ANY_PROTO_UPB_H_

#include "upb/msg_internal.h"
#include "upb/decode.h"
#include "upb/decode_fast.h"
#include "upb/encode.h"

#include "upb/port_def.inc"

#ifdef __cplusplus
extern "C" {
#endif

struct google_protobuf_Any;
typedef struct google_protobuf_Any google_protobuf_Any;
extern const upb_MiniTable google_protobuf_Any_msginit;



/* google.protobuf.Any */

UPB_INLINE google_protobuf_Any* google_protobuf_Any_new(upb_Arena* arena) {
  return (google_protobuf_Any*)_upb_Message_New(&google_protobuf_Any_msginit, arena);
}
UPB_INLINE google_protobuf_Any* google_protobuf_Any_parse(const char* buf, size_t size, upb_Arena* arena) {
  google_protobuf_Any* ret = google_protobuf_Any_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_Any_msginit, NULL, 0, arena) != kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE google_protobuf_Any* google_protobuf_Any_parse_ex(const char* buf, size_t size,
                           const upb_ExtensionRegistry* extreg,
                           int options, upb_Arena* arena) {
  google_protobuf_Any* ret = google_protobuf_Any_new(arena);
  if (!ret) return NULL;
  if (upb_Decode(buf, size, ret, &google_protobuf_Any_msginit, extreg, options, arena) !=
      kUpb_DecodeStatus_Ok) {
    return NULL;
  }
  return ret;
}
UPB_INLINE char* google_protobuf_Any_serialize(const google_protobuf_Any* msg, upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_Any_msginit, 0, arena, len);
}
UPB_INLINE char* google_protobuf_Any_serialize_ex(const google_protobuf_Any* msg, int options,
                                 upb_Arena* arena, size_t* len) {
  return upb_Encode(msg, &google_protobuf_Any_msginit, options, arena, len);
}
UPB_INLINE upb_StringView google_protobuf_Any_type_url(const google_protobuf_Any* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(0, 0), upb_StringView);
}
UPB_INLINE upb_StringView google_protobuf_Any_value(const google_protobuf_Any* msg) {
  return *UPB_PTR_AT(msg, UPB_SIZE(8, 16), upb_StringView);
}

UPB_INLINE void google_protobuf_Any_set_type_url(google_protobuf_Any *msg, upb_StringView value) {
  *UPB_PTR_AT(msg, UPB_SIZE(0, 0), upb_StringView) = value;
}
UPB_INLINE void google_protobuf_Any_set_value(google_protobuf_Any *msg, upb_StringView value) {
  *UPB_PTR_AT(msg, UPB_SIZE(8, 16), upb_StringView) = value;
}

extern const upb_MiniTable_File google_protobuf_any_proto_upb_file_layout;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#include "upb/port_undef.inc"

#endif  /* GOOGLE_PROTOBUF_ANY_PROTO_UPB_H_ */
