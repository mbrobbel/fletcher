FetchContent_Declare(tinyxml2
  GIT_REPOSITORY  https://github.com/leethomason/tinyxml2.git
  GIT_TAG         master
)
FetchContent_MakeAvailable(tinyxml2)

add_subdirectory(cerata)
add_subdirectory(fletchgen)
