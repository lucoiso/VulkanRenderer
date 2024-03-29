# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRenderer

# ----------- Global Definitions -----------
SET(LIBRARY_NAME SharedUtils)

# ------------- Library Setup --------------
SET(PRIVATE_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Private)
SET(PUBLIC_MODULES_BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Source/Public)

SET(PRIVATE_MODULES
        ${PRIVATE_MODULES_BASE_DIRECTORY}/Utils.cpp
)

SET(PUBLIC_MODULES
        ${PUBLIC_MODULES_BASE_DIRECTORY}/Utils.hpp
)

ADD_LIBRARY(${LIBRARY_NAME} ${PRIVATE_MODULES})
SET_TARGET_PROPERTIES(${LIBRARY_NAME} PROPERTIES LINKER_LANGUAGE CXX)

TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PRIVATE ${PRIVATE_MODULES_BASE_DIRECTORY})
TARGET_INCLUDE_DIRECTORIES(${LIBRARY_NAME} PUBLIC ${PUBLIC_MODULES_BASE_DIRECTORY})

TARGET_LINK_LIBRARIES(${LIBRARY_NAME} PUBLIC
        RenderCore
)