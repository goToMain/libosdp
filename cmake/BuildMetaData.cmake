execute_process(COMMAND
    git log --pretty=format:'%h' -n 1
    OUTPUT_VARIABLE GIT_REV
    ERROR_QUIET
)
string(STRIP "${GIT_REV}" GIT_REV)

if ("${GIT_REV}" STREQUAL "")
    set(GIT_REV "N/A")
    set(GIT_DIFF "")
    set(GIT_TAG "N/A")
    set(GIT_BRANCH "N/A")
    set(VERSION_STRING "0.0.0")
else()
    # GIT_REV
    string(SUBSTRING "${GIT_REV}" 1 7 GIT_REV)

    # GIT_DIFF
    execute_process(COMMAND
        bash -c "git diff --quiet --exit-code || echo +"
        OUTPUT_VARIABLE GIT_DIFF
    )
    string(STRIP "${GIT_DIFF}" GIT_DIFF)

    # GIT_TAG
    execute_process(COMMAND
        git describe --exact-match --tags
        OUTPUT_VARIABLE GIT_TAG ERROR_QUIET
    )
    string(STRIP "${GIT_TAG}" GIT_TAG)

    # GIT_BRANCH
    execute_process(COMMAND
        git rev-parse --abbrev-ref HEAD
        OUTPUT_VARIABLE GIT_BRANCH
    )
    string(STRIP "${GIT_BRANCH}" GIT_BRANCH)

    # VERSION_STRING
    execute_process(COMMAND
        git describe --tags --abbrev=0
        OUTPUT_VARIABLE GIT_TAG_VERSION ERROR_QUIET
    )
    string(STRIP "${GIT_TAG_VERSION}" GIT_TAG_VERSION)
    string(REGEX
        REPLACE "v?(([0-9]+)\\.([0-9]+)(\\.([0-9]+))?)" "\\1"
        VERSION_STRING ${GIT_TAG_VERSION}
    )
endif()

string(CONCAT BUILD_META_C
    "const char *GIT_REV = \"${GIT_REV}${GIT_DIFF}\";\n"
    "const char *GIT_TAG = \"${GIT_TAG}\";\n"
    "const char *GIT_BRANCH = \"${GIT_BRANCH}\";\n"
    "const char *VERSION = \"${VERSION_STRING}\";\n"
)

if(EXISTS ${CMAKE_BINARY_DIR}/build_meta_data.c)
    file(READ ${CMAKE_BINARY_DIR}/build_meta_data.c BUILD_META_C_OLD)
else()
    set(BUILD_META_C_OLD "")
endif()

if (NOT "${BUILD_META_C}" STREQUAL "${BUILD_META_C_OLD}")
    file(WRITE ${CMAKE_BINARY_DIR}/build_meta_data.c "${BUILD_META_C}")
endif()
