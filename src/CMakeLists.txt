function(prepend var prefix)
    set(listVar "")
    foreach(f ${ARGN})
        list(APPEND listVar "${prefix}/${f}")
    endforeach(f)
    set(${var} "${listVar}" PARENT_SCOPE)
endfunction(prepend)

prepend(SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}
    main.cpp
    avtools.cpp avtools.h
    mainwindow.cpp mainwindow.h mainwindow.ui
    mimetools.cpp mimetools.h
    mpdownloader.cpp mpdownloader.h
    unitsformat.cpp unitsformat.h
    ytscraper.cpp ytscraper.h
    yay.rc
)

set(SRC_FILES ${SRC_FILES} PARENT_SCOPE)
