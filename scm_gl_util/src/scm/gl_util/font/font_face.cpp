
#include "font_face.h"

#include <exception>
#include <stdexcept>
#include <sstream>

#include <boost/filesystem.hpp>
//#include <boost/tuple/tuple.hpp>

#include <scm/gl_core/render_device.h>
#include <scm/gl_core/texture_objects.h>

#include <scm/gl_util/font/detail/freetype_types.h>

namespace scm {
namespace gl {

namespace detail {

bool
check_file(const std::string& file_name)
{
    using namespace boost::filesystem;

    path file_path = path(file_name);
    if (!exists(file_path)) {
        return (false);
    }
    if (is_directory(file_path)) {
        return (false);
    }

    return (true);
}

void
find_font_style_files(const std::string&         in_regular_font_file,
                      std::vector<std::string>&  out_font_style_files)
{
    using namespace boost::filesystem;

    out_font_style_files.clear();
    out_font_style_files.resize(font_face::style_count);

    // insert regular style
    out_font_style_files[font_face::style_regular] = in_regular_font_file;

    path            font_file_path = path(in_regular_font_file);
    std::string     font_file_ext  = extension(font_file_path);
    std::string     font_file_base = basename(font_file_path);
    path            font_file_dir  = font_file_path.branch_path();

    // search for italic style
    path    font_file_italic = font_file_dir
                             / (font_file_base + std::string("i") + font_file_ext);
    if (exists(font_file_italic) && !is_directory(font_file_italic)) {
        out_font_style_files[font_face::style_italic] = font_file_italic.string();
    }

    // search for bold style
    path    font_file_bold = font_file_dir
                           / (font_file_base + std::string("b") + font_file_ext);
    if (exists(font_file_bold) && !is_directory(font_file_bold)) {
        out_font_style_files[font_face::style_bold] = font_file_bold.string();
    }
    else {
        font_file_bold =  font_file_dir
                        / (font_file_base + std::string("bd") + font_file_ext);
        if (exists(font_file_bold) && !is_directory(font_file_bold)) {
            out_font_style_files[font_face::style_bold] = font_file_bold.string();
        }
    }

    // search for bold italic style (z or bi name addition)
    path    font_file_bold_italic = font_file_dir
                                  / (font_file_base + std::string("z") + font_file_ext);
    if (exists(font_file_bold_italic) && !is_directory(font_file_bold_italic)) {
        out_font_style_files[font_face::style_bold_italic] = font_file_bold_italic.string();
    }
    else {
        font_file_bold_italic = font_file_dir
                              / (font_file_base + std::string("bi") + font_file_ext);
        if (exists(font_file_bold_italic) && !is_directory(font_file_bold_italic)) {
            out_font_style_files[font_face::style_bold_italic] = font_file_bold_italic.string();
        }
    }
}

} // namesapce detail

font_face::font_face(const render_device_ptr& device,
                     const std::string&       font_file,
                     unsigned                 point_size,
                     unsigned                 display_dpi)
  : _font_styles(style_count)
  , _font_styles_available(style_count)
  , _size_at_72dpi(0)
{
    using namespace scm::gl;
    using namespace scm::math;

    try {
        detail::ft_library  ft_lib;
        if (!ft_lib.open()) {
            std::ostringstream s;
            s << "font_face::font_face(): unable to initialize freetype library.";
            throw(std::runtime_error(s.str()));
        }

        if (!detail::check_file(font_file)) {
            std::ostringstream s;
            s << "font_face::font_face(): "
              << "font file missing or is a directory ('" << font_file << "')";
            throw(std::runtime_error(s.str()));
        }

        std::vector<std::string>    font_style_files;
        detail::find_font_style_files(font_file, font_style_files);

        // fill font styles
        math::vec2ui max_glyph_size(0u, 0u); // to store the maximal glyph size over all styles
        for (int i = 0; i < style_count; ++i) {
            _font_styles_available[i] = !font_style_files[i].empty();
            std::string cur_font_file = _font_styles_available[i] ? font_style_files[i] : font_style_files[0];

            detail::ft_face     ft_font;

            if (!ft_font.open_face(ft_lib, cur_font_file)) {
                std::ostringstream s;
                s << "font_face::font_face(): error loading font file ('" << cur_font_file << "')";
                throw(std::runtime_error(s.str()));
            }

            if (FT_Set_Char_Size(ft_font.get_face(), 0, point_size << 6, 0, display_dpi) != 0) {
                std::ostringstream s;
                s << "font_face::font_face(): unable to set character size (font: " << cur_font_file << ", size: " << point_size << ")";
                throw(std::runtime_error(s.str()));
            }
            // retrieve the maximal bounding box of all glyphs in the face
            vec2f  font_bbox_x;
            vec2f  font_bbox_y;

            if (ft_font.get_face()->face_flags & FT_FACE_FLAG_SCALABLE) {
                float   em_size = 1.0f * ft_font.get_face()->units_per_EM;
                float   x_scale = ft_font.get_face()->size->metrics.x_ppem / em_size;
                float   y_scale = ft_font.get_face()->size->metrics.y_ppem / em_size;

                font_bbox_x = vec2f(ft_font.get_face()->bbox.xMin * x_scale,
                                    ft_font.get_face()->bbox.xMax * x_scale);
                font_bbox_y = vec2f(ft_font.get_face()->bbox.yMin * y_scale,
                                    ft_font.get_face()->bbox.yMax * y_scale);

                _font_styles[i]._line_spacing        = static_cast<unsigned>(ceil(ft_font.get_face()->height * y_scale));
                _font_styles[i]._underline_position  = static_cast<int>(round(ft_font.get_face()->underline_position * y_scale));
                _font_styles[i]._underline_thickness = static_cast<unsigned>(round(ft_font.get_face()->underline_thickness * y_scale));


            }
            else if (ft_font.get_face()->face_flags & FT_FACE_FLAG_FIXED_SIZES) {
                font_bbox_x = vec2f(0.0f, static_cast<float>(ft_font.get_face()->size->metrics.max_advance >> 6));
                font_bbox_y = vec2f(0.0f, static_cast<float>(ft_font.get_face()->size->metrics.height >> 6));

                _font_styles[i]._line_spacing        = static_cast<int>(font_bbox_y.y);
                _font_styles[i]._underline_position  = -1;
                _font_styles[i]._underline_thickness = 1;
            }
            else {
                std::ostringstream s;
                s << "font_face::font_face(): invalid font face flags (not FT_FACE_FLAG_SCALABLE or FT_FACE_FLAG_FIXED_SIZES), "
                  << "(font: " << cur_font_file << ", size: " << point_size << ")";
                throw(std::runtime_error(s.str()));
            }

            vec2ui font_size = vec2ui(static_cast<unsigned>(ceil(font_bbox_x.y) - floor(font_bbox_x.x)),
                                      static_cast<unsigned>(ceil(font_bbox_y.y) - floor(font_bbox_y.x)));
            max_glyph_size.x = max<unsigned>(max_glyph_size.x, font_size.x);
            max_glyph_size.y = max<unsigned>(max_glyph_size.y, font_size.y);

            // calculate kerning information
            _font_styles[i]._kerning_table.resize(boost::extents[256][256]);

            if (ft_font.get_face()->face_flags & FT_FACE_FLAG_KERNING) {
                for (unsigned l = 0; l < 256; ++l) {
                    FT_UInt l_glyph_index = FT_Get_Char_Index(ft_font.get_face(), l);
                    for (unsigned r = 0; r < 256; ++r) {
                        FT_UInt     r_glyph_index = FT_Get_Char_Index(ft_font.get_face(), r);
                        FT_Vector   delta;
                        FT_Get_Kerning(ft_font.get_face(), l_glyph_index, r_glyph_index, FT_KERNING_DEFAULT, &delta);
                        _font_styles[i]._kerning_table[l][r] = static_cast<char>(delta.x >> 6);
                    }
                }
            }
            else {
                for (unsigned l = 0; l < 256; ++l) {
                    for (unsigned r = 0; r < 256; ++r) {
                        _font_styles[i]._kerning_table[l][r] = 0;
                    }
                }
            }
        }
        // end fill font styles

        // generate texture image
        // currently only supported is grey (1byte per pixel, mono fonts are also converted to grey)
        typedef vec<unsigned char, 2> glyph_texel; // 2 components (core, border... TO BE DONE!, currently only first used)
        vec3ui                        glyph_texture_dim  = vec3ui(max_glyph_size * 16, style_count); // a 16x16 grid of 256 glyphs in 4 layers
        size_t                        glyph_texture_size = static_cast<size_t>(glyph_texture_dim.x) * glyph_texture_dim.y * glyph_texture_dim.z;
        scoped_array<glyph_texel>     glyph_texture(new glyph_texel[glyph_texture_size]);
        memset(glyph_texture.get(), 0u, glyph_texture_size * 2); // clear to black

        for (int i = 0; i < style_count; ++i) {
            std::string cur_font_file = _font_styles_available[i] ? font_style_files[i] : font_style_files[0];
            _font_styles[i]._glyphs = glyph_container(256);

            detail::ft_face     ft_font;

            if (!ft_font.open_face(ft_lib, cur_font_file)) {
                std::ostringstream s;
                s << "font_face::font_face(): error loading font file ('" << cur_font_file << "')";
                throw(std::runtime_error(s.str()));
            }

            if (FT_Set_Char_Size(ft_font.get_face(), 0, point_size << 6, 0, display_dpi) != 0) {
                std::ostringstream s;
                s << "font_face::font_face(): "
                  << "unable to set character size (font: " << cur_font_file << ", size: " << point_size << ")";
                throw(std::runtime_error(s.str()));
            }

            for (unsigned c = 0; c < 256; ++c) {
                glyph_info&      cur_glyph = _font_styles[i]._glyphs[c];

                if(FT_Load_Glyph(ft_font.get_face(), FT_Get_Char_Index(ft_font.get_face(), c),
                                 //FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL)) {
                                 FT_LOAD_FORCE_AUTOHINT | FT_LOAD_TARGET_LIGHT)) {
                    continue;
                }
                if (FT_Render_Glyph(ft_font.get_face()->glyph,  FT_RENDER_MODE_LIGHT)) { // FT_RENDER_MODE_NORMAL)) { // 
                    continue;
                }
                FT_Bitmap& bitmap = ft_font.get_face()->glyph->bitmap;

                // calculate the glyphs grid position in the font texture array
                vec3ui tex_array_dst;
                tex_array_dst.x = (c & 0x0F) * max_glyph_size.x;
                tex_array_dst.y = glyph_texture_dim.y - ((c >> 4) + 1) * max_glyph_size.y;
                tex_array_dst.z = i;

                vec2f actual_glyph_bbox(static_cast<float>(bitmap.width) / glyph_texture_dim.x,
                                        static_cast<float>(bitmap.rows) / glyph_texture_dim.y);
                cur_glyph._tex_lower_left   = vec2f(static_cast<float>(tex_array_dst.x) / glyph_texture_dim.x,
                                                    static_cast<float>(tex_array_dst.y) / glyph_texture_dim.y);
                cur_glyph._tex_upper_right  = cur_glyph._tex_lower_left + actual_glyph_bbox;

                if (ft_font.get_face()->face_flags & FT_FACE_FLAG_SCALABLE) {
                    // linearHoriAdvance contains the 16.16 representation of the horizontal advance
                    // horiAdvance contains only the rounded advance which can be off by 1 and
                    // lead to sub styles beeing rendered to narrow
                    cur_glyph._advance          =  FT_CeilFix(ft_font.get_face()->glyph->linearHoriAdvance) >> 16;
                }
                else if (ft_font.get_face()->face_flags & FT_FACE_FLAG_FIXED_SIZES) {
                    cur_glyph._advance          = ft_font.get_face()->glyph->metrics.horiAdvance >> 6;
                }
                cur_glyph._bearing          = vec2i(   ft_font.get_face()->glyph->metrics.horiBearingX >> 6,
                                                      (ft_font.get_face()->glyph->metrics.horiBearingY >> 6)
                                                    - (ft_font.get_face()->glyph->metrics.height >> 6));
                // fill texture
                switch (bitmap.pixel_mode) {
                    case FT_PIXEL_MODE_GRAY:
                        for (int dy = 0; dy < bitmap.rows; ++dy) {
                            unsigned src_off = dy * bitmap.pitch;
                            //unsigned dst_off = dst_x + (dst_y + bitmap.rows - 1 - dy) * glyph_texture_dim.x;
                            unsigned dst_off =    tex_array_dst.x
                                               + (tex_array_dst.y + bitmap.rows - 1 - dy) * glyph_texture_dim.x
                                               + i * (glyph_texture_dim.x * glyph_texture_dim.y);
                            for (int dx = 0; dx < bitmap.width; ++dx) {
                                glyph_texture[dst_off + dx][0] = bitmap.buffer[src_off + dx];
                            }
                            //memcpy(glyph_texture.get() + dst_off, bitmap.buffer + src_off, bitmap.width);
                        }
                        break;
                    case FT_PIXEL_MODE_MONO:
                        for (int dy = 0; dy < bitmap.rows; ++dy) {
                            for (int dx = 0; dx < bitmap.pitch; ++dx) {
                                unsigned        src_off     = dx + dy * bitmap.pitch;
                                unsigned char   src_byte    = bitmap.buffer[src_off];
                                for (int bx = 0; bx < 8; ++bx) {
                                    unsigned        dst_off =   (tex_array_dst.x + dx * 8 + bx)
                                                              + (tex_array_dst.y + bitmap.rows - 1 - dy) * glyph_texture_dim.x
                                                              + i * (glyph_texture_dim.x * glyph_texture_dim.y);
                                    unsigned char   src_set = src_byte & (0x80 >> bx);
                                    unsigned char*  plah    = &src_byte;
                                    glyph_texture[dst_off][0] = src_set ? 255u : 0u;
                                }
                            }
                        }
                        break;
                    default:
                        continue;
                }
            }
        }
        // end generate texture image
        std::vector<void*> image_array_data_raw;
        image_array_data_raw.push_back(glyph_texture.get());

        _font_styles_texture_array = device->create_texture_2d(vec2ui(glyph_texture_dim.x, glyph_texture_dim.y),
                                                               FORMAT_RG_8, 1, glyph_texture_dim.z, 1,
                                                               FORMAT_RG_8, image_array_data_raw);

        if (!_font_styles_texture_array) {
            std::ostringstream s;
            s << "font_face::font_face(): unable to create texture object.";
            throw(std::runtime_error(s.str()));
        }

        image_array_data_raw.clear();
        glyph_texture.reset();
    }
    catch(...) {
        cleanup();
        throw;
    }
}

font_face::~font_face()
{
    cleanup();
}

const std::string&
font_face::name() const
{
    return (_name);
}

unsigned
font_face::size_at_72dpi() const
{
    return (_size_at_72dpi);
}

bool
font_face::has_style(style_type s) const
{
    return (_font_styles_available[s]);
}

const font_face::glyph_info&
font_face::glyph(char c, style_type s) const
{
    return (_font_styles[s]._glyphs[c]);
}

unsigned
font_face::line_spacing(style_type s) const
{
    return (_font_styles[s]._line_spacing);
}

int
font_face::kerning(char l, char r, style_type s) const
{
    return (_font_styles[s]._kerning_table[l][r]);
}

int
font_face::underline_position(style_type s) const
{
    return (_font_styles[s]._underline_position);
}

int
font_face::underline_thickness(style_type s) const
{
    return (_font_styles[s]._underline_thickness);
}

void
font_face::cleanup()
{
    _font_styles.clear();
    _font_styles_available.clear();
    _font_styles_texture_array.reset();
}

const texture_2d_ptr&
font_face::styles_texture_array() const
{
    return (_font_styles_texture_array);
}

} // namespace gl
} // namespace scm



















#if 0
#include <stdexcept>

#include <scm/core.h>

namespace scm {
namespace gl_classic {

// gl_font_face
face::face()
{
}

face::~face()
{
    cleanup_textures();
}

const texture_2d_rect& face::get_glyph_texture(font::face::style_type style) const
{
    style_textur_container::const_iterator style_it = _style_textures.find(style);

    if (style_it == _style_textures.end()) {
        style_it = _style_textures.find(font::face::regular);

        if (style_it == _style_textures.end()) {
            std::stringstream output;

            output << "scm::gl_classic::face::get_glyph(): "
                   << "unable to retrieve requested style (id = '" << style << "') "
                   << "fallback to regular style failed!" << std::endl;

            scm::err() << log::error
                       << output.str();

            throw std::runtime_error(output.str());
        }
    }

    return (*style_it->second.get());
}

void face::cleanup_textures()
{
    //style_textur_container::iterator style_it;

    //for (style_it  = _style_textures.begin();
    //     style_it != _style_textures.end();
    //     ++style_it) {
    //    style_it->second.reset();
    //}
    _style_textures.clear();
}

} // namespace gl_classic
} // namespace scm
#endif