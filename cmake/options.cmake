option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(HP_TCP_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(HP_TCP_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(HP_TCP_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(HP_TCP_ENABLE_ASAN AND HP_TCP_ENABLE_TSAN)
  message(FATAL_ERROR "ASan and TSan cannot be enabled together")
endif()
