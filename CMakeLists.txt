cmake_minimum_required(VERSION 3.5)

project(ffmpeg_demo)

# third library
include_directories(${PROJECT_SOURCE_DIR}/ffmpeg_build/include)
link_directories(${PROJECT_SOURCE_DIR}/ffmpeg_build/lib)

include_directories(${PROJECT_SOURCE_DIR}/third/opengl/include)
link_directories(${PROJECT_SOURCE_DIR}/third/opengl/libs)

include_directories(${PROJECT_SOURCE_DIR}/third/glad/include)
link_directories(${PROJECT_SOURCE_DIR}/third/glad/libs)

# ffmpeg dependency
list(APPEND FFMPEG_DEP m z lzma bz2 x264 mp3lame avcodec swresample avdevice avfilter avformat avutil swscale)

# 临时图片处理
include_directories(${PROJECT_SOURCE_DIR}/src)

# opengl/glad dependency
list(APPEND OPENGL_DEP dl glfw glad GL X11 pthread Xrandr Xi)

add_executable(demo ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(demo PUBLIC ${FFMPEG_DEP} ${OPENGL_DEP})