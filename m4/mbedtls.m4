dnl ######################################################################
dnl mbedtls support
AC_DEFUN([CHECK_MBEDTLS], [

case "$with_mbedtls" in
check|yes)
	case "$have_mbedtls" in
	yes) ;;
	*)
		save_LIBS="$LIBS"
		LIBS=""
		MBEDTLS_LIBS=""
		# clear cache
		unset ac_cv_search_mbedtls_ssl_init
		AC_SEARCH_LIBS([mbedtls_ssl_init], [mbedtls],
			[have_mbedtls=yes
			MBEDTLS_LIBS="$LIBS -lmbedtls -lmbedcrypto -lmbedx509"],
			[have_mbedtls=no],
			[-lmbedtls -lmbedcrypto -lmbedx509])
		LIBS="$save_LIBS"
		;;
	esac

	CPPFLAGS_SAVE=$CPPFLAGS
	CPPFLAGS="$CPPFLAGS $MBEDTLS_INCS"
	AC_CHECK_HEADERS([mbedtls/ssl.h], [], [have_mbedtls=no])
	CPPFLAGS=$CPPFLAGS_SAVE
	AC_SUBST(MBEDTLS_INCS)
	AC_SUBST(MBEDTLS_LIBS)
	if test "$have_mbedtls" = "yes" ; then
		AC_DEFINE(HAVE_MBEDTLS, 1, [Define if the system has mbedtls])
		AM_CONDITIONAL([HAVE_MBEDTLS], [true])
		have_crypto=yes
	elif test "$with_mbedtls" = "yes" ; then
		AC_MSG_ERROR([--with-mbedtls was given but MBedTLS not found])
	fi
	;;
esac

# check if we have and should use mbedtls
AM_CONDITIONAL(HAVE_MBEDTLS, [test "$have_mbedtls" = "yes"])
])
