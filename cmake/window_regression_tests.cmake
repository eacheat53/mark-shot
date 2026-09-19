# 1. 【窗口测试】【真实入口】复用应用对象，分别检查贴图交互和历史窗口
foreach(window_test IN ITEMS pinned-text-selection capture-history-window)
    string(REPLACE "-" "_" source_name "${window_test}")
    qt_add_executable(mark-shot-${window_test}-test
        tests/${source_name}_test.cpp
        "$<FILTER:$<TARGET_OBJECTS:mark-shot>,EXCLUDE,/main\\.cpp\\.(o|obj)$>"
    )
    target_include_directories(mark-shot-${window_test}-test PRIVATE "$<TARGET_PROPERTY:mark-shot,INCLUDE_DIRECTORIES>")
    target_compile_definitions(mark-shot-${window_test}-test PRIVATE "$<TARGET_PROPERTY:mark-shot,COMPILE_DEFINITIONS>")
    target_link_libraries(mark-shot-${window_test}-test PRIVATE "$<TARGET_PROPERTY:mark-shot,LINK_LIBRARIES>" Qt6::Test)
    get_target_property(window_test_standard mark-shot CXX_STANDARD)
    set_target_properties(mark-shot-${window_test}-test PROPERTIES CXX_STANDARD ${window_test_standard})
    if(WIN32)
        set_target_properties(mark-shot-${window_test}-test PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
    add_dependencies(mark-shot-${window_test}-test mark-shot)
    add_test(NAME ${window_test} COMMAND mark-shot-${window_test}-test)
    set_tests_properties(${window_test} PROPERTIES TIMEOUT 30
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_STYLE_OVERRIDE=Fusion;QT_SCALE_FACTOR=1")
endforeach()
