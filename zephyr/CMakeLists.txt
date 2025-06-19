if (CONFIG_LIBOSDP)
	set(OSDP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
	set(REPO_ROOT ${OSDP_ROOT})

	list(APPEND CMAKE_MODULE_PATH "${OSDP_ROOT}/cmake")
	include(GitInfo)

	## TODO: Convert these to Kconfig entries
	configure_file(${OSDP_ROOT}/src/osdp_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/osdp_config.h @ONLY)

	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/include/osdp_export.h "
		#ifndef OSDP_EXPORT_H_
		#define OSDP_EXPORT_H_

		#define OSDP_EXPORT
		#define OSDP_NO_EXPORT
		#define OSDP_DEPRECATED_EXPORT

		#endif /* OSDP_EXPORT_H_ */"
	)

	zephyr_interface_library_named(osdp)
	target_link_libraries(zephyr_interface INTERFACE osdp)
	target_include_directories(osdp INTERFACE ${OSDP_ROOT}/include)
	target_include_directories(osdp INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/include)

	zephyr_library_named(modules_osdp)
	zephyr_library_compile_definitions(__BARE_METAL__)
	zephyr_library_link_libraries(osdp)
	target_include_directories(modules_osdp PRIVATE ${OSDP_ROOT}/utils/include)

	zephyr_library_sources(
		## LibOSDP
		${OSDP_ROOT}/src/osdp_cp.c
		${OSDP_ROOT}/src/osdp_pd.c
		${OSDP_ROOT}/src/osdp_common.c
		${OSDP_ROOT}/src/osdp_phy.c
		${OSDP_ROOT}/src/osdp_sc.c
		${OSDP_ROOT}/src/osdp_file.c
		${OSDP_ROOT}/src/crypto/mbedtls.c

		## Utils
		## TODO: Migrate these to use (or wrap around) zephyr API
		${OSDP_ROOT}/utils/src/list.c
		${OSDP_ROOT}/utils/src/queue.c
		${OSDP_ROOT}/utils/src/slab.c
		${OSDP_ROOT}/utils/src/utils.c
		${OSDP_ROOT}/utils/src/logger.c
		${OSDP_ROOT}/utils/src/disjoint_set.c

		## Zephyr glue code
		src/osdp.c
	)

	target_include_directories(modules_osdp PRIVATE ${ZEPHYR_MBEDTLS_MODULE_DIR}/include)
	zephyr_link_libraries(mbedTLS)
endif()
