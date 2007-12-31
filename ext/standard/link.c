/*
   +----------------------------------------------------------------------+
   | PHP Version 5                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2008 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author:                                                              |
   +----------------------------------------------------------------------+
 */

/* $Id$ */

#include "php.h"
#include "php_filestat.h"
#include "php_globals.h"

#ifdef HAVE_SYMLINK

#include <stdlib.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/stat.h>
#include <string.h>
#if HAVE_PWD_H
#ifdef PHP_WIN32
#include "win32/pwd.h"
#else
#include <pwd.h>
#endif
#endif
#if HAVE_GRP_H
#ifdef PHP_WIN32
#include "win32/grp.h"
#else
#include <grp.h>
#endif
#endif
#include <errno.h>
#include <ctype.h>

#include "php_link.h"
#include "ext/standard/file.h"

/* {{{ proto string readlink(string filename) U
   Return the target of a symbolic link */
PHP_FUNCTION(readlink)
{
	zval **pp_link;
	char *link;
	UChar *target;
	int link_len, target_len;
	char buff[MAXPATHLEN];
	int ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Z", &pp_link) == FAILURE ||
		php_stream_path_param_encode(pp_link, &link, &link_len, REPORT_ERRORS, FG(default_context)) == FAILURE) {
		return;
	}

	if (php_check_open_basedir(link TSRMLS_CC)) {
		RETURN_FALSE;
	}

	ret = readlink(link, buff, MAXPATHLEN-1);

	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}
	/* Append NULL to the end of the string */
	buff[ret] = '\0';

	if (UG(unicode)) {
		if (SUCCESS == php_stream_path_decode(NULL, &target, &target_len, buff, strlen(buff), REPORT_ERRORS, FG(default_context))) {
			RETURN_UNICODEL(target, target_len, 0);
		} else {
			RETURN_FALSE;
		}
	} else {
		RETURN_STRING(buff, 1);
	}
}
/* }}} */

/* {{{ proto int linkinfo(string filename) U
   Returns the st_dev field of the UNIX C stat structure describing the link */
PHP_FUNCTION(linkinfo)
{
	zval **pp_link;
	char *link;
	int link_len;
	struct stat sb;
	int ret;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "Z", &pp_link) == FAILURE ||
		php_stream_path_param_encode(pp_link, &link, &link_len, REPORT_ERRORS, FG(default_context)) == FAILURE) {
		return;
	}

	ret = VCWD_LSTAT(link, &sb);
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_LONG(-1L);
	}

	RETURN_LONG((long) sb.st_dev);
}
/* }}} */

/* {{{ proto int symlink(string target, string link) U
   Create a symbolic link */
PHP_FUNCTION(symlink)
{
	zval **pp_topath, **pp_frompath;
	char *topath, *frompath;
	int topath_len, frompath_len;
	int ret;
	char source_p[MAXPATHLEN];
	char dest_p[MAXPATHLEN];


	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ZZ", &pp_topath, &pp_frompath) == FAILURE ||
		php_stream_path_param_encode(pp_topath, &topath, &topath_len, REPORT_ERRORS, FG(default_context)) == FAILURE ||
		php_stream_path_param_encode(pp_frompath, &frompath, &frompath_len, REPORT_ERRORS, FG(default_context)) == FAILURE) {
		return;
	}

	if (!expand_filepath(frompath, source_p TSRMLS_CC) || !expand_filepath(topath, dest_p TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	if (php_stream_locate_url_wrapper(source_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ||
		php_stream_locate_url_wrapper(dest_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ) 
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to symlink to a URL");
		RETURN_FALSE;
	}

	if (php_check_open_basedir(dest_p TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(source_p TSRMLS_CC)) {
		RETURN_FALSE;
	}

#ifndef ZTS
	ret = symlink(topath, frompath);
#else 
	ret = symlink(dest_p, source_p);
#endif	
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto int link(string target, string link) U
   Create a hard link */
PHP_FUNCTION(link)
{
	zval **pp_topath, **pp_frompath;
	char *topath, *frompath;
	int topath_len, frompath_len;
	int ret;
	char source_p[MAXPATHLEN];
	char dest_p[MAXPATHLEN];

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ZZ", &pp_topath, &pp_frompath) == FAILURE ||
		php_stream_path_param_encode(pp_topath, &topath, &topath_len, REPORT_ERRORS, FG(default_context)) == FAILURE ||
		php_stream_path_param_encode(pp_frompath, &frompath, &frompath_len, REPORT_ERRORS, FG(default_context)) == FAILURE) {
		return;
	}

	if (!expand_filepath(frompath, source_p TSRMLS_CC) || !expand_filepath(topath, dest_p TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "No such file or directory");
		RETURN_FALSE;
	}

	if (php_stream_locate_url_wrapper(source_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ||
		php_stream_locate_url_wrapper(dest_p, NULL, STREAM_LOCATE_WRAPPERS_ONLY TSRMLS_CC) ) 
	{
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to link to a URL");
		RETURN_FALSE;
	}

	if (php_check_open_basedir(dest_p TSRMLS_CC)) {
		RETURN_FALSE;
	}

	if (php_check_open_basedir(source_p TSRMLS_CC)) {
		RETURN_FALSE;
	}

#ifndef ZTS
	ret = link(topath, frompath);
#else 
	ret = link(dest_p, source_p);	
#endif	
	if (ret == -1) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", strerror(errno));
		RETURN_FALSE;
	}

	RETURN_TRUE;
}
/* }}} */

#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
