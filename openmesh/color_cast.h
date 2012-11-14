// Work around a bug in OpenMesh color casting
#pragma once

#include <other/core/openmesh/TriMesh.h>
namespace OpenMesh {

template <> struct color_caster<other::OVec<uint8_t,4>,Vec3f> {
  typedef other::OVec<uint8_t,4> return_type;
  static return_type cast(const Vec3f& src) {
    return return_type(uint8_t(255*src[0]),uint8_t(255*src[1]),uint8_t(255*src[2]),255);
  }
};
template <> struct color_caster<other::OVec<uint8_t,4>,Vec3uc> {
  typedef other::OVec<uint8_t,4> return_type;
  static return_type cast(const Vec3uc& src) {
    return return_type(src[0],src[1],src[2],255);
  }
};
template <> struct color_caster<other::OVec<uint8_t,4>,Vec4uc> {
  typedef other::OVec<uint8_t,4> return_type;
  static return_type cast(const Vec4uc& src) {
    return return_type(src[0],src[1],src[2],src[3]);
  }
};
template <> struct color_caster<Vec3uc,other::OVec<uint8_t,4>> {
  typedef Vec3uc return_type;
  static return_type cast(const other::OVec<uint8_t,4>& src) {
    return return_type(src[0],src[1],src[2]);
  }
};
template <> struct color_caster<Vec4uc,other::OVec<uint8_t,4>> {
  typedef Vec4uc return_type;
  static return_type cast(const other::OVec<uint8_t,4>& src) {
    return return_type(src[0],src[1],src[2],src[3]);
  }
};

}
