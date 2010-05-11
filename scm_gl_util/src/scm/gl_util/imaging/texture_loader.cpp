
#include "texture_loader.h"

#include <vector>

#include <freeimage/FreeImagePlus.h>

#include <scm/core/math.h>
#include <scm/core/pointer_types.h>

#include <scm/gl_core/data_formats.h>
#include <scm/gl_core/log.h>
#include <scm/gl_core/render_device.h>
#include <scm/gl_core/texture_objects.h>

namespace scm {
namespace gl {
namespace {

void scale_colors(float r, float g, float b,
                  int w, int h, data_format format,
                  void* data)
{
    unsigned channels = channel_count(format);

    if (   format == FORMAT_RGB_8
        || format == FORMAT_RGBA_8) {
        unsigned char* cdata = reinterpret_cast<unsigned char*>(data);
        for (unsigned i = 0; i < (w * h * channels); i += channels) {
            cdata[i]       = static_cast<unsigned char>(cdata[i]     * r);
            cdata[i + 1]   = static_cast<unsigned char>(cdata[i + 1] * g);
            cdata[i + 2]   = static_cast<unsigned char>(cdata[i + 2] * b);
        }
    }
    if (   format == FORMAT_BGR_8
        || format == FORMAT_BGRA_8) {
        unsigned char* cdata = reinterpret_cast<unsigned char*>(data);
        for (unsigned i = 0; i < (w * h * channels); i += channels) {
            cdata[i]       = static_cast<unsigned char>(cdata[i]     * b);
            cdata[i + 1]   = static_cast<unsigned char>(cdata[i + 1] * g);
            cdata[i + 2]   = static_cast<unsigned char>(cdata[i + 2] * r);
        }
    }
    if (   format == FORMAT_RGB_32F
        || format == FORMAT_RGBA_32F) {
        float* cdata = reinterpret_cast<float*>(data);
        for (unsigned i = 0; i < (w * h * channels); i += channels) {
            cdata[i]       = cdata[i]     * r;
            cdata[i + 1]   = cdata[i + 1] * g;
            cdata[i + 2]   = cdata[i + 2] * b;
        }
    }
}

} // namespace

texture_2d_ptr
texture_loader::load_texture_2d(render_device&       in_device,
                                const std::string&   in_image_path,
                                bool                 in_create_mips,
                                bool                 in_color_mips)
{
    scm::scoped_ptr<fipImage>   in_image(new fipImage);

    if (!in_image->load(in_image_path.c_str())) {
        glerr() << log::error << "texture_loader::load_texture_2d(): "
                << "unable to open file: " << in_image_path << log::end;
        return (texture_2d_ptr());
    }

    FREE_IMAGE_TYPE image_type = in_image->getImageType();
    math::vec2ui    image_size(in_image->getWidth(), in_image->getHeight());
    data_format     image_format = FORMAT_NULL;
    data_format     internal_format = FORMAT_NULL;
    
    switch (image_type) {
        case FIT_BITMAP: {
            unsigned num_components = in_image->getBitsPerPixel() / 8;
            switch (num_components) {
                case 1: image_format = internal_format = FORMAT_R_8; break;
                case 2: image_format = internal_format = FORMAT_RG_8; break;
                case 3: image_format = FORMAT_BGR_8; internal_format = FORMAT_RGB_8; break;
                case 4: image_format = FORMAT_BGRA_8; internal_format = FORMAT_RGBA_8; break;
            }
        } break;
        case FIT_INT16:     image_format = internal_format = FORMAT_R_16S; break;
        case FIT_UINT16:    image_format = internal_format = FORMAT_R_16; break;
        case FIT_RGB16:     image_format = internal_format = FORMAT_RGB_16; break;
        case FIT_RGBA16:    image_format = internal_format = FORMAT_RGBA_16; break;
        case FIT_INT32:     break; 
        case FIT_UINT32:    break;
        case FIT_FLOAT:     image_format = internal_format = FORMAT_R_32F; break;
        case FIT_RGBF:      image_format = internal_format = FORMAT_RGB_32F; break;
        case FIT_RGBAF:     image_format = internal_format = FORMAT_RGBA_32F; break;
    }

    if (image_format == FORMAT_NULL) {
        glerr() << log::error << "texture_loader::load_texture_2d(): "
                << "unsupported color format: " << std::hex << in_image->getImageType() << log::end;
        return (texture_2d_ptr());
    }

    std::vector<shared_array<unsigned char> >   image_mip_data;
    std::vector<void*>                          image_mip_data_raw;

    unsigned num_mip_levels = 1;
    if (in_create_mips) {
        num_mip_levels = util::max_mip_levels(image_size);
    }

    for (unsigned i = 0; i < num_mip_levels; ++i) {
        scm::size_t  cur_data_size = 0;
        math::vec2ui lev_size = util::mip_level_dimensions(image_size, i);

        if (i == 0) {
            cur_data_size =   image_size.x * image_size.y;
            cur_data_size *=  channel_count(image_format);
            cur_data_size *=  size_of_channel(image_format);
        }
        else {
            cur_data_size =   lev_size.x * lev_size.y;
            cur_data_size *=  channel_count(image_format);
            cur_data_size *=  size_of_channel(image_format);

            if (FALSE == in_image->rescale(lev_size.x, lev_size.y, FILTER_LANCZOS3)) {
                glerr() << log::error << "texture_loader::load_texture_2d(): "
                        << "unable to scale image (level: " << i << ", dim: " << lev_size << ")" << log::end;
                return (texture_2d_ptr());
            }
            if (image_type != in_image->getImageType()) {
                glerr() << log::error << "texture_loader::load_texture_2d(): "
                        << "image type changed after resamling (level: " << i
                        << ", dim: " << lev_size 
                        << ", type: " << std::hex << in_image->getImageType() << ")" << log::end;
                return (texture_2d_ptr());
            }
        }

        scm::shared_array<unsigned char> cur_data(new unsigned char[cur_data_size]);

        if (memcpy(cur_data.get(), in_image->accessPixels(), cur_data_size) != cur_data.get()) {
            glerr() << log::error << "texture_loader::load_texture_2d(): "
                    << "unable to copy image data (level: " << i << ", size: " << cur_data_size << "byte)" << log::end;
            return (texture_2d_ptr());
        }
        if (0 != i && in_color_mips) {
            if (i % 6 == 1) {
                scale_colors(1, 0, 0, lev_size.x, lev_size.y, image_format, cur_data.get());
            }
            else if (i % 6 == 2) {
                scale_colors(0, 1, 0, lev_size.x, lev_size.y, image_format, cur_data.get());
            }
            else if (i % 6 == 3) {
                scale_colors(0, 0, 1, lev_size.x, lev_size.y, image_format, cur_data.get());
            }
            else if (i % 6 == 4) {
                scale_colors(1, 0, 1, lev_size.x, lev_size.y, image_format, cur_data.get());
            }
            else if (i % 6 == 5) {
                scale_colors(0, 1, 1, lev_size.x, lev_size.y, image_format, cur_data.get());
            }
            else if (i % 6 == 0) {
                scale_colors(1, 1, 0, lev_size.x, lev_size.y, image_format, cur_data.get());
            }
        }

        image_mip_data.push_back(cur_data);
        image_mip_data_raw.push_back(cur_data.get());
    }

    texture_2d_ptr new_tex = in_device.create_texture_2d(image_size, internal_format, num_mip_levels, 1, 1,
                                                         image_format, image_mip_data_raw);

    if (!new_tex) {
        glerr() << log::error << "texture_loader::load_texture_2d(): "
                << "unable to create texture object (file: " << in_image_path << ")" << log::end;
        return (texture_2d_ptr());
    }

    image_mip_data_raw.clear();
    image_mip_data.clear();

    return (new_tex);
}

} // namespace gl
} // namespace scm
