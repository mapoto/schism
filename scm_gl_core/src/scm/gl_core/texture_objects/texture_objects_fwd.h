
#ifndef SCM_GL_CORE_TEXTURE_OBJECTS_FWD_H_INCLUDED
#define SCM_GL_CORE_TEXTURE_OBJECTS_FWD_H_INCLUDED

#include <scm/core/pointer_types.h>

namespace scm {
namespace gl {

class texture;

struct texture_1d_desc;
class  texture_1d;

struct texture_2d_desc;
class  texture_2d;

struct texture_3d_desc;
class  texture_3d;

typedef shared_ptr<texture>             texture_ptr;
typedef shared_ptr<texture_1d>          texture_1d_ptr;
typedef shared_ptr<texture_2d>          texture_2d_ptr;
typedef shared_ptr<texture_3d>          texture_3d_ptr;

} // namespace gl
} // namespace scm

#endif // SCM_GL_CORE_TEXTURE_OBJECTS_FWD_H_INCLUDED
