# Hand-maintained per-target / per-file tweaks applied after the generated
# cmake/targets.cmake. Keep each entry commented with the reason.
#
# blur_source_options(<target> <file-regex> <option>...) adds compile options
# to the sources of <target> whose path matches <file-regex>. It looks the
# file up in the target's source list so the generated path spelling
# (jam-era capitalisation) does not matter.
function(blur_source_options tgt rx)
    get_target_property(_srcs ${tgt} SOURCES)
    foreach(_s IN LISTS _srcs)
        if(_s MATCHES "${rx}")
            set_property(SOURCE ${_s} APPEND PROPERTY COMPILE_OPTIONS ${ARGN})
        endif()
    endforeach()
endfunction()

# libtomcrypt 1.10: tomcrypt.h defines SHA1/SHA256 descriptor macros, and the
# Windows 11 SDK winnt.h (IMAGE_HOT_PATCH_HASHES) has fields named SHA1/SHA256.
# rng_get_bytes.c includes <windows.h> after tomcrypt.h; force windows.h first.
blur_source_options(libtomcryptv "rng_get_bytes\\.c$" /FIwindows.h)

# libjpeg 6b's jmorecfg.h typedefs INT32 unless XMD_H is defined; the Windows
# SDK (basetsd.h) already has INT32, and JpegHelper includes both.
target_compile_definitions(JpegHelper PRIVATE XMD_H)
