set(XChainServicePath ${WAVM_SOURCE_DIR}/ThirdParty)

set(Sources
    Emscripten.cpp
    Interface.cpp
)
set(PublicHeaders
    ${WAVM_INCLUDE_DIR}/Emscripten/Emscripten.h
    ${WAVM_INCLUDE_DIR}/Emscripten/xchain-service.h
    ${WAVM_INCLUDE_DIR}/Emscripten/Interface.h
)

WAVM_ADD_LIB_COMPONENT(Emscripten
	SOURCES ${Sources} ${PublicHeaders}
    PRIVATE_INCLUDE_DIRECTORIES ${XChainServicePath}
    PUBLIC_LIBS  ${WAVM_SOURCE_DIR}/output/ThirdParty/bridge/libbridge.a
    PRIVATE_LIB_COMPONENTS Logging Platform WASM WASTParse bridge)
