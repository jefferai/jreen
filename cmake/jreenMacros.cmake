

if(SYMBIAN)
	function(jreen_add_executable)
		symbian_add_executable(${ARGN})
	endfunction()
	function(jreen_target_link_libraries)
		symbian_target_link_libraries(${ARGN})
	endfunction()
else(SYMBIAN)
	function(jreen_add_executable)
		add_executable(${ARGN})
	endfunction()
	function(jreen_target_link_libraries)
		target_link_libraries(${ARGN})
	endfunction()
endif(SYMBIAN)

MACRO (JREEN_WRAP_CPP outfiles )
	# get include dirs
	QT4_GET_MOC_FLAGS(moc_flags)
	QT4_EXTRACT_OPTIONS(moc_files moc_options ${ARGN})

	FOREACH (it ${moc_files})
		GET_FILENAME_COMPONENT(_abs_FILE ${it} ABSOLUTE)
		GET_FILENAME_COMPONENT(_abs_PATH ${_abs_FILE} PATH)
		GET_FILENAME_COMPONENT(_basename ${it} NAME_WE)

		SET(_HAS_MOC false)

		IF(EXISTS ${_abs_PATH}/${_basename}.cpp)
			SET(_header ${_abs_PATH}/${_basename}.cpp)
			FILE(READ ${_header} _contents)
			STRING(REGEX MATCHALL "# *include +[^ ]+\\.moc[\">]" _match "${_contents}")
			IF(_match)
				SET(_HAS_MOC true)
				FOREACH (_current_MOC_INC ${_match})
					STRING(REGEX MATCH "[^ <\"]+\\.moc" _current_MOC "${_current_MOC_INC}")
					SET(_moc    ${CMAKE_CURRENT_BINARY_DIR}/${_current_MOC})
					QT4_CREATE_MOC_COMMAND(${_abs_FILE} ${_moc} "${_moc_INCS}" "")
					MACRO_ADD_FILE_DEPENDENCIES(${_abs_FILE} ${_moc})
				ENDFOREACH (_current_MOC_INC)
			ENDIF()
		ENDIF()
		IF(NOT _HAS_MOC)
			FILE(READ ${_abs_FILE} _contents)
			STRING(REGEX MATCHALL "Q_OBJECT" _match2 "${_contents}")
			IF(_match2)
				QT4_MAKE_OUTPUT_FILE(${_abs_FILE} moc_ cxx outfile)
				QT4_CREATE_MOC_COMMAND(${_abs_FILE} ${outfile} "${moc_flags}" "${moc_options}")
				SET(${outfiles} ${${outfiles}} ${outfile})
			ENDIF()
		ENDIF()
	ENDFOREACH(it)
ENDMACRO (JREEN_WRAP_CPP)
