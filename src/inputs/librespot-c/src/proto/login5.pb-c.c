/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: login5.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "login5.pb-c.h"
void   spotify__login5__v3__challenges__init
                     (Spotify__Login5__V3__Challenges         *message)
{
  static const Spotify__Login5__V3__Challenges init_value = SPOTIFY__LOGIN5__V3__CHALLENGES__INIT;
  *message = init_value;
}
size_t spotify__login5__v3__challenges__get_packed_size
                     (const Spotify__Login5__V3__Challenges *message)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenges__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t spotify__login5__v3__challenges__pack
                     (const Spotify__Login5__V3__Challenges *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenges__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t spotify__login5__v3__challenges__pack_to_buffer
                     (const Spotify__Login5__V3__Challenges *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenges__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Spotify__Login5__V3__Challenges *
       spotify__login5__v3__challenges__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Spotify__Login5__V3__Challenges *)
     protobuf_c_message_unpack (&spotify__login5__v3__challenges__descriptor,
                                allocator, len, data);
}
void   spotify__login5__v3__challenges__free_unpacked
                     (Spotify__Login5__V3__Challenges *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &spotify__login5__v3__challenges__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   spotify__login5__v3__challenge__init
                     (Spotify__Login5__V3__Challenge         *message)
{
  static const Spotify__Login5__V3__Challenge init_value = SPOTIFY__LOGIN5__V3__CHALLENGE__INIT;
  *message = init_value;
}
size_t spotify__login5__v3__challenge__get_packed_size
                     (const Spotify__Login5__V3__Challenge *message)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t spotify__login5__v3__challenge__pack
                     (const Spotify__Login5__V3__Challenge *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t spotify__login5__v3__challenge__pack_to_buffer
                     (const Spotify__Login5__V3__Challenge *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Spotify__Login5__V3__Challenge *
       spotify__login5__v3__challenge__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Spotify__Login5__V3__Challenge *)
     protobuf_c_message_unpack (&spotify__login5__v3__challenge__descriptor,
                                allocator, len, data);
}
void   spotify__login5__v3__challenge__free_unpacked
                     (Spotify__Login5__V3__Challenge *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &spotify__login5__v3__challenge__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   spotify__login5__v3__challenge_solutions__init
                     (Spotify__Login5__V3__ChallengeSolutions         *message)
{
  static const Spotify__Login5__V3__ChallengeSolutions init_value = SPOTIFY__LOGIN5__V3__CHALLENGE_SOLUTIONS__INIT;
  *message = init_value;
}
size_t spotify__login5__v3__challenge_solutions__get_packed_size
                     (const Spotify__Login5__V3__ChallengeSolutions *message)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solutions__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t spotify__login5__v3__challenge_solutions__pack
                     (const Spotify__Login5__V3__ChallengeSolutions *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solutions__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t spotify__login5__v3__challenge_solutions__pack_to_buffer
                     (const Spotify__Login5__V3__ChallengeSolutions *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solutions__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Spotify__Login5__V3__ChallengeSolutions *
       spotify__login5__v3__challenge_solutions__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Spotify__Login5__V3__ChallengeSolutions *)
     protobuf_c_message_unpack (&spotify__login5__v3__challenge_solutions__descriptor,
                                allocator, len, data);
}
void   spotify__login5__v3__challenge_solutions__free_unpacked
                     (Spotify__Login5__V3__ChallengeSolutions *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solutions__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   spotify__login5__v3__challenge_solution__init
                     (Spotify__Login5__V3__ChallengeSolution         *message)
{
  static const Spotify__Login5__V3__ChallengeSolution init_value = SPOTIFY__LOGIN5__V3__CHALLENGE_SOLUTION__INIT;
  *message = init_value;
}
size_t spotify__login5__v3__challenge_solution__get_packed_size
                     (const Spotify__Login5__V3__ChallengeSolution *message)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solution__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t spotify__login5__v3__challenge_solution__pack
                     (const Spotify__Login5__V3__ChallengeSolution *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solution__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t spotify__login5__v3__challenge_solution__pack_to_buffer
                     (const Spotify__Login5__V3__ChallengeSolution *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solution__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Spotify__Login5__V3__ChallengeSolution *
       spotify__login5__v3__challenge_solution__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Spotify__Login5__V3__ChallengeSolution *)
     protobuf_c_message_unpack (&spotify__login5__v3__challenge_solution__descriptor,
                                allocator, len, data);
}
void   spotify__login5__v3__challenge_solution__free_unpacked
                     (Spotify__Login5__V3__ChallengeSolution *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &spotify__login5__v3__challenge_solution__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   spotify__login5__v3__login_request__init
                     (Spotify__Login5__V3__LoginRequest         *message)
{
  static const Spotify__Login5__V3__LoginRequest init_value = SPOTIFY__LOGIN5__V3__LOGIN_REQUEST__INIT;
  *message = init_value;
}
size_t spotify__login5__v3__login_request__get_packed_size
                     (const Spotify__Login5__V3__LoginRequest *message)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_request__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t spotify__login5__v3__login_request__pack
                     (const Spotify__Login5__V3__LoginRequest *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_request__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t spotify__login5__v3__login_request__pack_to_buffer
                     (const Spotify__Login5__V3__LoginRequest *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_request__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Spotify__Login5__V3__LoginRequest *
       spotify__login5__v3__login_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Spotify__Login5__V3__LoginRequest *)
     protobuf_c_message_unpack (&spotify__login5__v3__login_request__descriptor,
                                allocator, len, data);
}
void   spotify__login5__v3__login_request__free_unpacked
                     (Spotify__Login5__V3__LoginRequest *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &spotify__login5__v3__login_request__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   spotify__login5__v3__login_ok__init
                     (Spotify__Login5__V3__LoginOk         *message)
{
  static const Spotify__Login5__V3__LoginOk init_value = SPOTIFY__LOGIN5__V3__LOGIN_OK__INIT;
  *message = init_value;
}
size_t spotify__login5__v3__login_ok__get_packed_size
                     (const Spotify__Login5__V3__LoginOk *message)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_ok__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t spotify__login5__v3__login_ok__pack
                     (const Spotify__Login5__V3__LoginOk *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_ok__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t spotify__login5__v3__login_ok__pack_to_buffer
                     (const Spotify__Login5__V3__LoginOk *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_ok__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Spotify__Login5__V3__LoginOk *
       spotify__login5__v3__login_ok__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Spotify__Login5__V3__LoginOk *)
     protobuf_c_message_unpack (&spotify__login5__v3__login_ok__descriptor,
                                allocator, len, data);
}
void   spotify__login5__v3__login_ok__free_unpacked
                     (Spotify__Login5__V3__LoginOk *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &spotify__login5__v3__login_ok__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   spotify__login5__v3__login_response__init
                     (Spotify__Login5__V3__LoginResponse         *message)
{
  static const Spotify__Login5__V3__LoginResponse init_value = SPOTIFY__LOGIN5__V3__LOGIN_RESPONSE__INIT;
  *message = init_value;
}
size_t spotify__login5__v3__login_response__get_packed_size
                     (const Spotify__Login5__V3__LoginResponse *message)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_response__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t spotify__login5__v3__login_response__pack
                     (const Spotify__Login5__V3__LoginResponse *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_response__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t spotify__login5__v3__login_response__pack_to_buffer
                     (const Spotify__Login5__V3__LoginResponse *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &spotify__login5__v3__login_response__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
Spotify__Login5__V3__LoginResponse *
       spotify__login5__v3__login_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (Spotify__Login5__V3__LoginResponse *)
     protobuf_c_message_unpack (&spotify__login5__v3__login_response__descriptor,
                                allocator, len, data);
}
void   spotify__login5__v3__login_response__free_unpacked
                     (Spotify__Login5__V3__LoginResponse *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &spotify__login5__v3__login_response__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor spotify__login5__v3__challenges__field_descriptors[1] =
{
  {
    "challenges",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__Challenges, n_challenges),
    offsetof(Spotify__Login5__V3__Challenges, challenges),
    &spotify__login5__v3__challenge__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned spotify__login5__v3__challenges__field_indices_by_name[] = {
  0,   /* field[0] = challenges */
};
static const ProtobufCIntRange spotify__login5__v3__challenges__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor spotify__login5__v3__challenges__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.Challenges",
  "Challenges",
  "Spotify__Login5__V3__Challenges",
  "spotify.login5.v3",
  sizeof(Spotify__Login5__V3__Challenges),
  1,
  spotify__login5__v3__challenges__field_descriptors,
  spotify__login5__v3__challenges__field_indices_by_name,
  1,  spotify__login5__v3__challenges__number_ranges,
  (ProtobufCMessageInit) spotify__login5__v3__challenges__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor spotify__login5__v3__challenge__field_descriptors[2] =
{
  {
    "hashcash",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__Challenge, challenge_case),
    offsetof(Spotify__Login5__V3__Challenge, hashcash),
    &spotify__login5__v3__challenges__hashcash_challenge__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "code",
    2,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__Challenge, challenge_case),
    offsetof(Spotify__Login5__V3__Challenge, code),
    &spotify__login5__v3__challenges__code_challenge__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned spotify__login5__v3__challenge__field_indices_by_name[] = {
  1,   /* field[1] = code */
  0,   /* field[0] = hashcash */
};
static const ProtobufCIntRange spotify__login5__v3__challenge__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor spotify__login5__v3__challenge__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.Challenge",
  "Challenge",
  "Spotify__Login5__V3__Challenge",
  "spotify.login5.v3",
  sizeof(Spotify__Login5__V3__Challenge),
  2,
  spotify__login5__v3__challenge__field_descriptors,
  spotify__login5__v3__challenge__field_indices_by_name,
  1,  spotify__login5__v3__challenge__number_ranges,
  (ProtobufCMessageInit) spotify__login5__v3__challenge__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor spotify__login5__v3__challenge_solutions__field_descriptors[1] =
{
  {
    "solutions",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__ChallengeSolutions, n_solutions),
    offsetof(Spotify__Login5__V3__ChallengeSolutions, solutions),
    &spotify__login5__v3__challenge_solution__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned spotify__login5__v3__challenge_solutions__field_indices_by_name[] = {
  0,   /* field[0] = solutions */
};
static const ProtobufCIntRange spotify__login5__v3__challenge_solutions__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor spotify__login5__v3__challenge_solutions__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.ChallengeSolutions",
  "ChallengeSolutions",
  "Spotify__Login5__V3__ChallengeSolutions",
  "spotify.login5.v3",
  sizeof(Spotify__Login5__V3__ChallengeSolutions),
  1,
  spotify__login5__v3__challenge_solutions__field_descriptors,
  spotify__login5__v3__challenge_solutions__field_indices_by_name,
  1,  spotify__login5__v3__challenge_solutions__number_ranges,
  (ProtobufCMessageInit) spotify__login5__v3__challenge_solutions__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor spotify__login5__v3__challenge_solution__field_descriptors[2] =
{
  {
    "hashcash",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__ChallengeSolution, solution_case),
    offsetof(Spotify__Login5__V3__ChallengeSolution, hashcash),
    &spotify__login5__v3__challenges__hashcash_solution__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "code",
    2,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__ChallengeSolution, solution_case),
    offsetof(Spotify__Login5__V3__ChallengeSolution, code),
    &spotify__login5__v3__challenges__code_solution__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned spotify__login5__v3__challenge_solution__field_indices_by_name[] = {
  1,   /* field[1] = code */
  0,   /* field[0] = hashcash */
};
static const ProtobufCIntRange spotify__login5__v3__challenge_solution__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor spotify__login5__v3__challenge_solution__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.ChallengeSolution",
  "ChallengeSolution",
  "Spotify__Login5__V3__ChallengeSolution",
  "spotify.login5.v3",
  sizeof(Spotify__Login5__V3__ChallengeSolution),
  2,
  spotify__login5__v3__challenge_solution__field_descriptors,
  spotify__login5__v3__challenge_solution__field_indices_by_name,
  1,  spotify__login5__v3__challenge_solution__number_ranges,
  (ProtobufCMessageInit) spotify__login5__v3__challenge_solution__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor spotify__login5__v3__login_request__field_descriptors[12] =
{
  {
    "client_info",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginRequest, client_info),
    &spotify__login5__v3__client_info__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "login_context",
    2,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_BYTES,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginRequest, login_context),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "challenge_solutions",
    3,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginRequest, challenge_solutions),
    &spotify__login5__v3__challenge_solutions__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "stored_credential",
    100,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, stored_credential),
    &spotify__login5__v3__credentials__stored_credential__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "password",
    101,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, password),
    &spotify__login5__v3__credentials__password__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "facebook_access_token",
    102,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, facebook_access_token),
    &spotify__login5__v3__credentials__facebook_access_token__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "phone_number",
    103,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, phone_number),
    &spotify__login5__v3__identifiers__phone_number__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "one_time_token",
    104,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, one_time_token),
    &spotify__login5__v3__credentials__one_time_token__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "parent_child_credential",
    105,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, parent_child_credential),
    &spotify__login5__v3__credentials__parent_child_credential__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "apple_sign_in_credential",
    106,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, apple_sign_in_credential),
    &spotify__login5__v3__credentials__apple_sign_in_credential__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "samsung_sign_in_credential",
    107,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, samsung_sign_in_credential),
    &spotify__login5__v3__credentials__samsung_sign_in_credential__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "google_sign_in_credential",
    108,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginRequest, login_method_case),
    offsetof(Spotify__Login5__V3__LoginRequest, google_sign_in_credential),
    &spotify__login5__v3__credentials__google_sign_in_credential__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned spotify__login5__v3__login_request__field_indices_by_name[] = {
  9,   /* field[9] = apple_sign_in_credential */
  2,   /* field[2] = challenge_solutions */
  0,   /* field[0] = client_info */
  5,   /* field[5] = facebook_access_token */
  11,   /* field[11] = google_sign_in_credential */
  1,   /* field[1] = login_context */
  7,   /* field[7] = one_time_token */
  8,   /* field[8] = parent_child_credential */
  4,   /* field[4] = password */
  6,   /* field[6] = phone_number */
  10,   /* field[10] = samsung_sign_in_credential */
  3,   /* field[3] = stored_credential */
};
static const ProtobufCIntRange spotify__login5__v3__login_request__number_ranges[2 + 1] =
{
  { 1, 0 },
  { 100, 3 },
  { 0, 12 }
};
const ProtobufCMessageDescriptor spotify__login5__v3__login_request__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.LoginRequest",
  "LoginRequest",
  "Spotify__Login5__V3__LoginRequest",
  "spotify.login5.v3",
  sizeof(Spotify__Login5__V3__LoginRequest),
  12,
  spotify__login5__v3__login_request__field_descriptors,
  spotify__login5__v3__login_request__field_indices_by_name,
  2,  spotify__login5__v3__login_request__number_ranges,
  (ProtobufCMessageInit) spotify__login5__v3__login_request__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor spotify__login5__v3__login_ok__field_descriptors[4] =
{
  {
    "username",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginOk, username),
    NULL,
    &protobuf_c_empty_string,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "access_token",
    2,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginOk, access_token),
    NULL,
    &protobuf_c_empty_string,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "stored_credential",
    3,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_BYTES,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginOk, stored_credential),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "access_token_expires_in",
    4,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginOk, access_token_expires_in),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned spotify__login5__v3__login_ok__field_indices_by_name[] = {
  1,   /* field[1] = access_token */
  3,   /* field[3] = access_token_expires_in */
  2,   /* field[2] = stored_credential */
  0,   /* field[0] = username */
};
static const ProtobufCIntRange spotify__login5__v3__login_ok__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor spotify__login5__v3__login_ok__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.LoginOk",
  "LoginOk",
  "Spotify__Login5__V3__LoginOk",
  "spotify.login5.v3",
  sizeof(Spotify__Login5__V3__LoginOk),
  4,
  spotify__login5__v3__login_ok__field_descriptors,
  spotify__login5__v3__login_ok__field_indices_by_name,
  1,  spotify__login5__v3__login_ok__number_ranges,
  (ProtobufCMessageInit) spotify__login5__v3__login_ok__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue spotify__login5__v3__login_response__warnings__enum_values_by_number[2] =
{
  { "UNKNOWN_WARNING", "SPOTIFY__LOGIN5__V3__LOGIN_RESPONSE__WARNINGS__UNKNOWN_WARNING", 0 },
  { "DEPRECATED_PROTOCOL_VERSION", "SPOTIFY__LOGIN5__V3__LOGIN_RESPONSE__WARNINGS__DEPRECATED_PROTOCOL_VERSION", 1 },
};
static const ProtobufCIntRange spotify__login5__v3__login_response__warnings__value_ranges[] = {
{0, 0},{0, 2}
};
static const ProtobufCEnumValueIndex spotify__login5__v3__login_response__warnings__enum_values_by_name[2] =
{
  { "DEPRECATED_PROTOCOL_VERSION", 1 },
  { "UNKNOWN_WARNING", 0 },
};
const ProtobufCEnumDescriptor spotify__login5__v3__login_response__warnings__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.LoginResponse.Warnings",
  "Warnings",
  "Spotify__Login5__V3__LoginResponse__Warnings",
  "spotify.login5.v3",
  2,
  spotify__login5__v3__login_response__warnings__enum_values_by_number,
  2,
  spotify__login5__v3__login_response__warnings__enum_values_by_name,
  1,
  spotify__login5__v3__login_response__warnings__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const ProtobufCFieldDescriptor spotify__login5__v3__login_response__field_descriptors[7] =
{
  {
    "ok",
    1,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginResponse, response_case),
    offsetof(Spotify__Login5__V3__LoginResponse, ok),
    &spotify__login5__v3__login_ok__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "error",
    2,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_ENUM,
    offsetof(Spotify__Login5__V3__LoginResponse, response_case),
    offsetof(Spotify__Login5__V3__LoginResponse, error),
    &spotify__login5__v3__login_error__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "challenges",
    3,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(Spotify__Login5__V3__LoginResponse, response_case),
    offsetof(Spotify__Login5__V3__LoginResponse, challenges),
    &spotify__login5__v3__challenges__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_ONEOF,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "warnings",
    4,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_ENUM,
    offsetof(Spotify__Login5__V3__LoginResponse, n_warnings),
    offsetof(Spotify__Login5__V3__LoginResponse, warnings),
    &spotify__login5__v3__login_response__warnings__descriptor,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_PACKED,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "login_context",
    5,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_BYTES,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginResponse, login_context),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "identifier_token",
    6,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_STRING,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginResponse, identifier_token),
    NULL,
    &protobuf_c_empty_string,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "user_info",
    7,
    PROTOBUF_C_LABEL_NONE,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(Spotify__Login5__V3__LoginResponse, user_info),
    &spotify__login5__v3__user_info__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned spotify__login5__v3__login_response__field_indices_by_name[] = {
  2,   /* field[2] = challenges */
  1,   /* field[1] = error */
  5,   /* field[5] = identifier_token */
  4,   /* field[4] = login_context */
  0,   /* field[0] = ok */
  6,   /* field[6] = user_info */
  3,   /* field[3] = warnings */
};
static const ProtobufCIntRange spotify__login5__v3__login_response__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 7 }
};
const ProtobufCMessageDescriptor spotify__login5__v3__login_response__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.LoginResponse",
  "LoginResponse",
  "Spotify__Login5__V3__LoginResponse",
  "spotify.login5.v3",
  sizeof(Spotify__Login5__V3__LoginResponse),
  7,
  spotify__login5__v3__login_response__field_descriptors,
  spotify__login5__v3__login_response__field_indices_by_name,
  1,  spotify__login5__v3__login_response__number_ranges,
  (ProtobufCMessageInit) spotify__login5__v3__login_response__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue spotify__login5__v3__login_error__enum_values_by_number[9] =
{
  { "UNKNOWN_ERROR", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__UNKNOWN_ERROR", 0 },
  { "INVALID_CREDENTIALS", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__INVALID_CREDENTIALS", 1 },
  { "BAD_REQUEST", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__BAD_REQUEST", 2 },
  { "UNSUPPORTED_LOGIN_PROTOCOL", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__UNSUPPORTED_LOGIN_PROTOCOL", 3 },
  { "TIMEOUT", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__TIMEOUT", 4 },
  { "UNKNOWN_IDENTIFIER", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__UNKNOWN_IDENTIFIER", 5 },
  { "TOO_MANY_ATTEMPTS", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__TOO_MANY_ATTEMPTS", 6 },
  { "INVALID_PHONENUMBER", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__INVALID_PHONENUMBER", 7 },
  { "TRY_AGAIN_LATER", "SPOTIFY__LOGIN5__V3__LOGIN_ERROR__TRY_AGAIN_LATER", 8 },
};
static const ProtobufCIntRange spotify__login5__v3__login_error__value_ranges[] = {
{0, 0},{0, 9}
};
static const ProtobufCEnumValueIndex spotify__login5__v3__login_error__enum_values_by_name[9] =
{
  { "BAD_REQUEST", 2 },
  { "INVALID_CREDENTIALS", 1 },
  { "INVALID_PHONENUMBER", 7 },
  { "TIMEOUT", 4 },
  { "TOO_MANY_ATTEMPTS", 6 },
  { "TRY_AGAIN_LATER", 8 },
  { "UNKNOWN_ERROR", 0 },
  { "UNKNOWN_IDENTIFIER", 5 },
  { "UNSUPPORTED_LOGIN_PROTOCOL", 3 },
};
const ProtobufCEnumDescriptor spotify__login5__v3__login_error__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "spotify.login5.v3.LoginError",
  "LoginError",
  "Spotify__Login5__V3__LoginError",
  "spotify.login5.v3",
  9,
  spotify__login5__v3__login_error__enum_values_by_number,
  9,
  spotify__login5__v3__login_error__enum_values_by_name,
  1,
  spotify__login5__v3__login_error__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
