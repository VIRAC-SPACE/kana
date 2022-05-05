cmake_minimum_required(VERSION  3.20.1)

project(Kana LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-O2 -Wall -Werror -g  -lfftw3f -lm -lvdifio")
set(CMAKE_INSTALL_PREFIX /usr/local/bin)

find_package(MPI REQUIRED COMPONENTS CXX)
find_package(PkgConfig REQUIRED)

pkg_search_module(FFTW REQUIRED fftw3 IMPORTED_TARGET)

include_directories(PkgConfig::FFTW)
include(CheckCXXCompilerFlag)

link_libraries(PkgConfig::FFTW)
link_libraries(PkgConfig::FFTW -lfftw3)

add_library(utils STATIC src/utils.c include/utils.h)
add_library(panic STATIC src/panic.c include/panic.h)
add_library(socket SHARED src/socket.c include/socket.h)
add_library(JSON_parser SHARED src/JSON_parser.c include/JSON_parser.h)
add_library(jsonReader STATIC src/jsonReader.c include/jsonReader.h)
add_library(hzCorrect STATIC freqCorrect/hzCorrect.c freqCorrect/hzCorrect.h)
add_library(coefficients SHARED src/coefficients.c include/coefficients.h)
add_library(lagCorrect STATIC timeCorrect/lagCorrect.c timeCorrect/lagCorrect.h)
add_library(spectra STATIC mode1/spectra.c mode1/spectra.h)
add_library(mode1 STATIC mode1/mode1.c mode1/mode1.h)
add_library(mode2 STATIC mode2/mode2.c mode2/mode2.h)
add_library(mode3 STATIC mode3/mode3.c mode3/mode3.h)

add_executable(dataSender dataSender/main.c)
add_executable(freqCorrect freqCorrect/main.c)
add_executable(timeCorrect timeCorrect/main.c)
add_executable(m1 mode1/main.c)
add_executable(m2 mode2/main.c)
add_executable(m3 mode3/main.c)


target_link_libraries(dataSender PUBLIC socket JSON_parser jsonReader utils panic)
target_link_libraries(freqCorrect PUBLIC JSON_parser jsonReader panic utils hzCorrect MPI::MPI_CXX -lm coefficients)
target_link_libraries(timeCorrect PUBLIC coefficients JSON_parser jsonReader panic utils MPI::MPI_CXX -lm  lagCorrect)
target_link_libraries(m1 PUBLIC socket JSON_parser jsonReader panic utils MPI::MPI_CXX -lm coefficients mode1 spectra -lvdifio  -lfftw3f)
target_link_libraries(m2 PUBLIC socket JSON_parser jsonReader panic utils MPI::MPI_CXX -lm coefficients mode2 -lvdifio  -lfftw3f)
target_link_libraries(m3 PUBLIC socket JSON_parser jsonReader panic utils MPI::MPI_CXX -lm coefficients mode3 -lvdifio  -lfftw3f)

install(TARGETS dataSender freqCorrect timeCorrect m1 m2 m3  DESTINATION /usr/local/bin)
install(TARGETS socket JSON_parser coefficients DESTINATION /usr/local/lib/)
