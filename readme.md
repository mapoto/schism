schism C++ libraries
====================
The schism C++ libraries offer object-oriented interfaces to the OpenGL C-languafe API with related interfaces for Nvidia CUDA and OpenCL compute language interoperability and various utilities to help with common real-time graphis related tasks. The libraries are written in standard C++ using OpenGL 3.3 to 4.2 core profile functionality and are useable on Windows as well as Linux operating systems.

The libraries were developed at the [Virtual Reality Systems Group at the Bauhaus-Universit�t Weimar][1] to support my PhD research work and are used in various student thesis and research projects at our group. Current projects include virtual reality applications and visualization software for extremely large image and especially volume data sets.

The main features of the schism C++ libraries are:
 * basic vector, matrix and quaternion classes and operations for real-time graphics context
 * basic graphics primitives and related intersection tests (ray, plane, box, frustum)
 * object-oriented interface to OpenGL core profile functionality
 * compute interoperability with OpenCL and Nvidia CUDA
 * window and context management for Windows (Win32) and Linux (X and GLX)
 * basic viewer window provived using Qt GUI-Framework
 * texure-based FreeType2 font rendering (sub-pixel rasterization, outlined drawing)
 * logging functionality (terminal and file output, extendable)
 * timing functionality for profiling tasks (non-blocking OpenGL, OpenCL, CUDA timers)
 * data loaders for image data (standard images through FreeImage and custom DDS loader)
 * data loaders for volume primitives (RAW, VoxelGeo, SEG-Y, DDS)

[1] http://www.uni-weimar.de/medien/vr

schism - external dependencies
------------------------------
For Windows the external dependencies need to be placed into the /externals folder. You can find precompiled versions of the required libraries using the [link below][1]. Download the externals package suitable for your platform and extract the contents into the /externals folder.

[2] schism externals: http://db.tt/8XSt6Rig

contributors
------------
The primary developer of the schism C++ libraries is [Christopher Lux](http://github.com/chrislu)