
add_library(gamma STATIC
  "${CMAKE_CURRENT_LIST_DIR}/Gamma/src/Conversion.cpp"
  # "${CMAKE_CURRENT_LIST_DIR}/Gamma/src/DFT.cpp"
  # "${CMAKE_CURRENT_LIST_DIR}/Gamma/src/FFT.cpp"
  # "${CMAKE_CURRENT_LIST_DIR}/Gamma/src/fftpack++1.cpp"
  # "${CMAKE_CURRENT_LIST_DIR}/Gamma/src/fftpack++2.cpp"
  # "${CMAKE_CURRENT_LIST_DIR}/Gamma/src/Domain.cpp"
  )

target_include_directories(
  gamma
  SYSTEM
  PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/Gamma"
)
