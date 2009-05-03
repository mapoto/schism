
#ifndef SCM_OGL_INDEXBUFFER_H_INCLUDED
#define SCM_OGL_INDEXBUFFER_H_INCLUDED

#include <cstddef>
#include <string>

#include <scm/gl/opengl.h>

#include <scm/gl/vertexbuffer_object/element_array.h>
#include <scm/gl/vertexbuffer_object/element_layout.h>

#include <scm/core/platform/platform.h>

namespace scm {
namespace gl {

class __scm_export(ogl) indexbuffer
{
public:
    indexbuffer();
    virtual ~indexbuffer();

    void                        bind() const;
    void                        draw_elements() const;
    void                        unbind() const;
                            
    void                        clear();
                            
    bool                        element_data(std::size_t            /*num_indices*/,
                                             const element_layout&  /*layout*/,
                                             const void*const       /*data*/,
                                             unsigned               /*usage_type*/ = GL_STATIC_DRAW);

    std::size_t                 num_indices() const;

    const gl::element_layout&   get_element_layout() const;

protected:
    element_array               _indices;

    std::size_t                 _num_indices;

    element_layout              _element_layout;

private:

}; // class indexbuffer

} // namespace gl
} // namespace scm

#endif // SCM_OGL_INDEXBUFFER_H_INCLUDED