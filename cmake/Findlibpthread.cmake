
set(PTHREAD_INCLUDE_DIR "/home/adam/b/pthread/Pre-built.2/include/")
set(PTHREAD_LIBRARY "/home/adam/b/pthread/Pre-built.2/dll/x86/pthreadGC2.dll")

find_package_handle_standard_args(PTHREAD
  DEFAULT_MSG
  PTHREAD_INCLUDE_DIR
  PTHREAD_LIBRARY
)

