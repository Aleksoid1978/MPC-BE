/*  Copyright (c) MediaArea.net SARL. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#ifndef MediaInfo_Reader_libcurl_IncludeH
#define MediaInfo_Reader_libcurl_IncludeH
//---------------------------------------------------------------------------

/* curl_off_t
 * ----------
 *
 * For any given platform/compiler curl_off_t must be typedef'ed to a 64-bit
 * wide signed integral data type. The width of this data type must remain
 * constant and independent of any possible large file support settings.
 */
typedef int64s curl_off_t;

//***************************************************************************
// Copy of curl include files - Easy interface
//***************************************************************************

typedef void CURL;

typedef enum {
    CURLE_OK = 0,
    CURLE_UNSUPPORTED_PROTOCOL,    /* 1 */
    CURLE_FAILED_INIT,             /* 2 */
    CURLE_URL_MALFORMAT,           /* 3 */
    CURLE_NOT_BUILT_IN,            /* 4 - [was obsoleted in August 2007 for
                                      7.17.0, reused in April 2011 for 7.21.5] */
    CURLE_COULDNT_RESOLVE_PROXY,   /* 5 */
    CURLE_COULDNT_RESOLVE_HOST,    /* 6 */
    CURLE_COULDNT_CONNECT,         /* 7 */
    CURLE_WEIRD_SERVER_REPLY,      /* 8 */
    CURLE_REMOTE_ACCESS_DENIED,    /* 9 a service was denied by the server
                                      due to lack of access - when login fails
                                      this is not returned. */
    CURLE_FTP_ACCEPT_FAILED,       /* 10 - [was obsoleted in April 2006 for
                                      7.15.4, reused in Dec 2011 for 7.24.0]*/
    CURLE_FTP_WEIRD_PASS_REPLY,    /* 11 */
    CURLE_FTP_ACCEPT_TIMEOUT,      /* 12 - timeout occurred accepting server
                                      [was obsoleted in August 2007 for 7.17.0,
                                      reused in Dec 2011 for 7.24.0]*/
    CURLE_FTP_WEIRD_PASV_REPLY,    /* 13 */
    CURLE_FTP_WEIRD_227_FORMAT,    /* 14 */
    CURLE_FTP_CANT_GET_HOST,       /* 15 */
    CURLE_HTTP2,                   /* 16 - A problem in the http2 framing layer.
                                      [was obsoleted in August 2007 for 7.17.0,
                                      reused in July 2014 for 7.38.0] */
    CURLE_FTP_COULDNT_SET_TYPE,    /* 17 */
    CURLE_PARTIAL_FILE,            /* 18 */
    CURLE_FTP_COULDNT_RETR_FILE,   /* 19 */
    CURLE_OBSOLETE20,              /* 20 - NOT USED */
    CURLE_QUOTE_ERROR,             /* 21 - quote command failure */
    CURLE_HTTP_RETURNED_ERROR,     /* 22 */
    CURLE_WRITE_ERROR,             /* 23 */
    CURLE_OBSOLETE24,              /* 24 - NOT USED */
    CURLE_UPLOAD_FAILED,           /* 25 - failed upload "command" */
    CURLE_READ_ERROR,              /* 26 - couldn't open/read from file */
    CURLE_OUT_OF_MEMORY,           /* 27 */
    /* Note: CURLE_OUT_OF_MEMORY may sometimes indicate a conversion error
             instead of a memory allocation error if CURL_DOES_CONVERSIONS
             is defined
    */
    CURLE_OPERATION_TIMEDOUT,      /* 28 - the timeout time was reached */
    CURLE_OBSOLETE29,              /* 29 - NOT USED */
    CURLE_FTP_PORT_FAILED,         /* 30 - FTP PORT operation failed */
    CURLE_FTP_COULDNT_USE_REST,    /* 31 - the REST command failed */
    CURLE_OBSOLETE32,              /* 32 - NOT USED */
    CURLE_RANGE_ERROR,             /* 33 - RANGE "command" didn't work */
    CURLE_HTTP_POST_ERROR,         /* 34 */
    CURLE_SSL_CONNECT_ERROR,       /* 35 - wrong when connecting with SSL */
    CURLE_BAD_DOWNLOAD_RESUME,     /* 36 - couldn't resume download */
    CURLE_FILE_COULDNT_READ_FILE,  /* 37 */
    CURLE_LDAP_CANNOT_BIND,        /* 38 */
    CURLE_LDAP_SEARCH_FAILED,      /* 39 */
    CURLE_OBSOLETE40,              /* 40 - NOT USED */
    CURLE_FUNCTION_NOT_FOUND,      /* 41 - NOT USED starting with 7.53.0 */
    CURLE_ABORTED_BY_CALLBACK,     /* 42 */
    CURLE_BAD_FUNCTION_ARGUMENT,   /* 43 */
    CURLE_OBSOLETE44,              /* 44 - NOT USED */
    CURLE_INTERFACE_FAILED,        /* 45 - CURLOPT_INTERFACE failed */
    CURLE_OBSOLETE46,              /* 46 - NOT USED */
    CURLE_TOO_MANY_REDIRECTS,      /* 47 - catch endless re-direct loops */
    CURLE_UNKNOWN_OPTION,          /* 48 - User specified an unknown option */
    CURLE_TELNET_OPTION_SYNTAX,    /* 49 - Malformed telnet option */
    CURLE_OBSOLETE50,              /* 50 - NOT USED */
    CURLE_PEER_FAILED_VERIFICATION, /* 51 - peer's certificate or fingerprint
                                       wasn't verified fine */
    CURLE_GOT_NOTHING,             /* 52 - when this is a specific error */
    CURLE_SSL_ENGINE_NOTFOUND,     /* 53 - SSL crypto engine not found */
    CURLE_SSL_ENGINE_SETFAILED,    /* 54 - can not set SSL crypto engine as
                                      default */
    CURLE_SEND_ERROR,              /* 55 - failed sending network data */
    CURLE_RECV_ERROR,              /* 56 - failure in receiving network data */
    CURLE_OBSOLETE57,              /* 57 - NOT IN USE */
    CURLE_SSL_CERTPROBLEM,         /* 58 - problem with the local certificate */
    CURLE_SSL_CIPHER,              /* 59 - couldn't use specified cipher */
    CURLE_SSL_CACERT,              /* 60 - problem with the CA cert (path?) */
    CURLE_BAD_CONTENT_ENCODING,    /* 61 - Unrecognized/bad encoding */
    CURLE_LDAP_INVALID_URL,        /* 62 - Invalid LDAP URL */
    CURLE_FILESIZE_EXCEEDED,       /* 63 - Maximum file size exceeded */
    CURLE_USE_SSL_FAILED,          /* 64 - Requested FTP SSL level failed */
    CURLE_SEND_FAIL_REWIND,        /* 65 - Sending the data requires a rewind
                                      that failed */
    CURLE_SSL_ENGINE_INITFAILED,   /* 66 - failed to initialise ENGINE */
    CURLE_LOGIN_DENIED,            /* 67 - user, password or similar was not
                                      accepted and we failed to login */
    CURLE_TFTP_NOTFOUND,           /* 68 - file not found on server */
    CURLE_TFTP_PERM,               /* 69 - permission problem on server */
    CURLE_REMOTE_DISK_FULL,        /* 70 - out of disk space on server */
    CURLE_TFTP_ILLEGAL,            /* 71 - Illegal TFTP operation */
    CURLE_TFTP_UNKNOWNID,          /* 72 - Unknown transfer ID */
    CURLE_REMOTE_FILE_EXISTS,      /* 73 - File already exists */
    CURLE_TFTP_NOSUCHUSER,         /* 74 - No such user */
    CURLE_CONV_FAILED,             /* 75 - conversion failed */
    CURLE_CONV_REQD,               /* 76 - caller must register conversion
                                      callbacks using curl_easy_setopt options
                                      CURLOPT_CONV_FROM_NETWORK_FUNCTION,
                                      CURLOPT_CONV_TO_NETWORK_FUNCTION, and
                                      CURLOPT_CONV_FROM_UTF8_FUNCTION */
    CURLE_SSL_CACERT_BADFILE,      /* 77 - could not load CACERT file, missing
                                      or wrong format */
    CURLE_REMOTE_FILE_NOT_FOUND,   /* 78 - remote file not found */
    CURLE_SSH,                     /* 79 - error from the SSH layer, somewhat
                                      generic so the error message will be of
                                      interest when this has happened */

    CURLE_SSL_SHUTDOWN_FAILED,     /* 80 - Failed to shut down the SSL
                                      connection */
    CURLE_AGAIN,                   /* 81 - socket is not ready for send/recv,
                                      wait till it's ready and try again (Added
                                      in 7.18.2) */
    CURLE_SSL_CRL_BADFILE,         /* 82 - could not load CRL file, missing or
                                      wrong format (Added in 7.19.0) */
    CURLE_SSL_ISSUER_ERROR,        /* 83 - Issuer check failed.  (Added in
                                      7.19.0) */
    CURLE_FTP_PRET_FAILED,         /* 84 - a PRET command failed */
    CURLE_RTSP_CSEQ_ERROR,         /* 85 - mismatch of RTSP CSeq numbers */
    CURLE_RTSP_SESSION_ERROR,      /* 86 - mismatch of RTSP Session Ids */
    CURLE_FTP_BAD_FILE_LIST,       /* 87 - unable to parse FTP file list */
    CURLE_CHUNK_FAILED,            /* 88 - chunk callback reported error */
    CURLE_NO_CONNECTION_AVAILABLE, /* 89 - No connection available, the
                                      session will be queued */
    CURLE_SSL_PINNEDPUBKEYNOTMATCH, /* 90 - specified pinned public key did not
                                       match */
    CURLE_SSL_INVALIDCERTSTATUS,   /* 91 - invalid certificate status */
    CURLE_HTTP2_STREAM,            /* 92 - stream error in HTTP/2 framing layer
                                      */
    CURL_LAST /* never use! */
} CURLcode;

#define CURLINFO_STRING   0x100000
#define CURLINFO_LONG     0x200000
#define CURLINFO_DOUBLE   0x300000
#define CURLINFO_SLIST    0x400000
#define CURLINFO_PTR      0x400000 /* same as SLIST */
#define CURLINFO_SOCKET   0x500000
#define CURLINFO_OFF_T    0x600000
#define CURLINFO_MASK     0x0fffff
#define CURLINFO_TYPEMASK 0xf00000

typedef enum {
  CURLINFO_NONE, /* first, never use this */
  CURLINFO_EFFECTIVE_URL    = CURLINFO_STRING + 1,
  CURLINFO_RESPONSE_CODE    = CURLINFO_LONG   + 2,
  CURLINFO_TOTAL_TIME       = CURLINFO_DOUBLE + 3,
  CURLINFO_NAMELOOKUP_TIME  = CURLINFO_DOUBLE + 4,
  CURLINFO_CONNECT_TIME     = CURLINFO_DOUBLE + 5,
  CURLINFO_PRETRANSFER_TIME = CURLINFO_DOUBLE + 6,
  CURLINFO_SIZE_UPLOAD      = CURLINFO_DOUBLE + 7,
  CURLINFO_SIZE_UPLOAD_T    = CURLINFO_OFF_T  + 7,
  CURLINFO_SIZE_DOWNLOAD    = CURLINFO_DOUBLE + 8,
  CURLINFO_SIZE_DOWNLOAD_T  = CURLINFO_OFF_T  + 8,
  CURLINFO_SPEED_DOWNLOAD   = CURLINFO_DOUBLE + 9,
  CURLINFO_SPEED_DOWNLOAD_T = CURLINFO_OFF_T  + 9,
  CURLINFO_SPEED_UPLOAD     = CURLINFO_DOUBLE + 10,
  CURLINFO_SPEED_UPLOAD_T   = CURLINFO_OFF_T  + 10,
  CURLINFO_HEADER_SIZE      = CURLINFO_LONG   + 11,
  CURLINFO_REQUEST_SIZE     = CURLINFO_LONG   + 12,
  CURLINFO_SSL_VERIFYRESULT = CURLINFO_LONG   + 13,
  CURLINFO_FILETIME         = CURLINFO_LONG   + 14,
  CURLINFO_CONTENT_LENGTH_DOWNLOAD   = CURLINFO_DOUBLE + 15,
  CURLINFO_CONTENT_LENGTH_DOWNLOAD_T = CURLINFO_OFF_T  + 15,
  CURLINFO_CONTENT_LENGTH_UPLOAD     = CURLINFO_DOUBLE + 16,
  CURLINFO_CONTENT_LENGTH_UPLOAD_T   = CURLINFO_OFF_T  + 16,
  CURLINFO_STARTTRANSFER_TIME = CURLINFO_DOUBLE + 17,
  CURLINFO_CONTENT_TYPE     = CURLINFO_STRING + 18,
  CURLINFO_REDIRECT_TIME    = CURLINFO_DOUBLE + 19,
  CURLINFO_REDIRECT_COUNT   = CURLINFO_LONG   + 20,
  CURLINFO_PRIVATE          = CURLINFO_STRING + 21,
  CURLINFO_HTTP_CONNECTCODE = CURLINFO_LONG   + 22,
  CURLINFO_HTTPAUTH_AVAIL   = CURLINFO_LONG   + 23,
  CURLINFO_PROXYAUTH_AVAIL  = CURLINFO_LONG   + 24,
  CURLINFO_OS_ERRNO         = CURLINFO_LONG   + 25,
  CURLINFO_NUM_CONNECTS     = CURLINFO_LONG   + 26,
  CURLINFO_SSL_ENGINES      = CURLINFO_SLIST  + 27,
  CURLINFO_COOKIELIST       = CURLINFO_SLIST  + 28,
  CURLINFO_LASTSOCKET       = CURLINFO_LONG   + 29,
  CURLINFO_FTP_ENTRY_PATH   = CURLINFO_STRING + 30,
  CURLINFO_REDIRECT_URL     = CURLINFO_STRING + 31,
  CURLINFO_PRIMARY_IP       = CURLINFO_STRING + 32,
  CURLINFO_APPCONNECT_TIME  = CURLINFO_DOUBLE + 33,
  CURLINFO_CERTINFO         = CURLINFO_PTR    + 34,
  CURLINFO_CONDITION_UNMET  = CURLINFO_LONG   + 35,
  CURLINFO_RTSP_SESSION_ID  = CURLINFO_STRING + 36,
  CURLINFO_RTSP_CLIENT_CSEQ = CURLINFO_LONG   + 37,
  CURLINFO_RTSP_SERVER_CSEQ = CURLINFO_LONG   + 38,
  CURLINFO_RTSP_CSEQ_RECV   = CURLINFO_LONG   + 39,
  CURLINFO_PRIMARY_PORT     = CURLINFO_LONG   + 40,
  CURLINFO_LOCAL_IP         = CURLINFO_STRING + 41,
  CURLINFO_LOCAL_PORT       = CURLINFO_LONG   + 42,
  CURLINFO_TLS_SESSION      = CURLINFO_PTR    + 43,
  CURLINFO_ACTIVESOCKET     = CURLINFO_SOCKET + 44,
  CURLINFO_TLS_SSL_PTR      = CURLINFO_PTR    + 45,
  CURLINFO_HTTP_VERSION     = CURLINFO_LONG   + 46,
  CURLINFO_PROXY_SSL_VERIFYRESULT = CURLINFO_LONG + 47,
  CURLINFO_PROTOCOL         = CURLINFO_LONG   + 48,
  CURLINFO_SCHEME           = CURLINFO_STRING + 49,
  /* Fill in new entries below here! */

  CURLINFO_LASTONE          = 49
} CURLINFO;

/* CURLPROTO_ defines are for the CURLOPT_*PROTOCOLS options */
#define CURLPROTO_HTTP   (1<<0)
#define CURLPROTO_HTTPS  (1<<1)
#define CURLPROTO_FTP    (1<<2)
#define CURLPROTO_FTPS   (1<<3)
#define CURLPROTO_SCP    (1<<4)
#define CURLPROTO_SFTP   (1<<5)
#define CURLPROTO_TELNET (1<<6)
#define CURLPROTO_LDAP   (1<<7)
#define CURLPROTO_LDAPS  (1<<8)
#define CURLPROTO_DICT   (1<<9)
#define CURLPROTO_FILE   (1<<10)
#define CURLPROTO_TFTP   (1<<11)
#define CURLPROTO_IMAP   (1<<12)
#define CURLPROTO_IMAPS  (1<<13)
#define CURLPROTO_POP3   (1<<14)
#define CURLPROTO_POP3S  (1<<15)
#define CURLPROTO_SMTP   (1<<16)
#define CURLPROTO_SMTPS  (1<<17)
#define CURLPROTO_RTSP   (1<<18)
#define CURLPROTO_RTMP   (1<<19)
#define CURLPROTO_RTMPT  (1<<20)
#define CURLPROTO_RTMPE  (1<<21)
#define CURLPROTO_RTMPTE (1<<22)
#define CURLPROTO_RTMPS  (1<<23)
#define CURLPROTO_RTMPTS (1<<24)
#define CURLPROTO_GOPHER (1<<25)
#define CURLPROTO_SMB    (1<<26)
#define CURLPROTO_SMBS   (1<<27)
#define CURLPROTO_ALL    (~0) /* enable everything */

/* long may be 32 or 64 bits, but we should never depend on anything else
   but 32 */
#define CURLOPTTYPE_LONG          0
#define CURLOPTTYPE_OBJECTPOINT   10000
#define CURLOPTTYPE_STRINGPOINT   10000
#define CURLOPTTYPE_FUNCTIONPOINT 20000
#define CURLOPTTYPE_OFF_T         30000

#define CINIT(name,type,number) CURLOPT_ ## name = CURLOPTTYPE_ ## type + number

/*
 * This macro-mania below setups the CURLOPT_[what] enum, to be used with
 * curl_easy_setopt(). The first argument in the CINIT() macro is the [what]
 * word.
 */

typedef enum {
  /* This is the FILE * or void * the regular output should be written to. */
  CINIT(WRITEDATA, OBJECTPOINT, 1),

  /* The full URL to get/put */
  CINIT(URL, STRINGPOINT, 2),

  /* Port number to connect to, if other than default. */
  CINIT(PORT, LONG, 3),

  /* Name of proxy to use. */
  CINIT(PROXY, STRINGPOINT, 4),

  /* "user:password;options" to use when fetching. */
  CINIT(USERPWD, STRINGPOINT, 5),

  /* "user:password" to use with proxy. */
  CINIT(PROXYUSERPWD, STRINGPOINT, 6),

  /* Range to get, specified as an ASCII string. */
  CINIT(RANGE, STRINGPOINT, 7),

  /* not used */

  /* Specified file stream to upload from (use as input): */
  CINIT(READDATA, OBJECTPOINT, 9),

  /* Buffer to receive error messages in, must be at least CURL_ERROR_SIZE
   * bytes big. If this is not used, error messages go to stderr instead: */
  CINIT(ERRORBUFFER, OBJECTPOINT, 10),

  /* Function that will be called to store the output (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CINIT(WRITEFUNCTION, FUNCTIONPOINT, 11),

  /* Function that will be called to read the input (instead of fread). The
   * parameters will use fread() syntax, make sure to follow them. */
  CINIT(READFUNCTION, FUNCTIONPOINT, 12),

  /* Time-out the read operation after this amount of seconds */
  CINIT(TIMEOUT, LONG, 13),

  /* If the CURLOPT_INFILE is used, this can be used to inform libcurl about
   * how large the file being sent really is. That allows better error
   * checking and better verifies that the upload was successful. -1 means
   * unknown size.
   *
   * For large file support, there is also a _LARGE version of the key
   * which takes an off_t type, allowing platforms with larger off_t
   * sizes to handle larger files.  See below for INFILESIZE_LARGE.
   */
  CINIT(INFILESIZE, LONG, 14),

  /* POST static input fields. */
  CINIT(POSTFIELDS, OBJECTPOINT, 15),

  /* Set the referrer page (needed by some CGIs) */
  CINIT(REFERER, STRINGPOINT, 16),

  /* Set the FTP PORT string (interface name, named or numerical IP address)
     Use i.e '-' to use default address. */
  CINIT(FTPPORT, STRINGPOINT, 17),

  /* Set the User-Agent string (examined by some CGIs) */
  CINIT(USERAGENT, STRINGPOINT, 18),

  /* If the download receives less than "low speed limit" bytes/second
   * during "low speed time" seconds, the operations is aborted.
   * You could i.e if you have a pretty high speed connection, abort if
   * it is less than 2000 bytes/sec during 20 seconds.
   */

  /* Set the "low speed limit" */
  CINIT(LOW_SPEED_LIMIT, LONG, 19),

  /* Set the "low speed time" */
  CINIT(LOW_SPEED_TIME, LONG, 20),

  /* Set the continuation offset.
   *
   * Note there is also a _LARGE version of this key which uses
   * off_t types, allowing for large file offsets on platforms which
   * use larger-than-32-bit off_t's.  Look below for RESUME_FROM_LARGE.
   */
  CINIT(RESUME_FROM, LONG, 21),

  /* Set cookie in request: */
  CINIT(COOKIE, STRINGPOINT, 22),

  /* This points to a linked list of headers, struct curl_slist kind. This
     list is also used for RTSP (in spite of its name) */
  CINIT(HTTPHEADER, OBJECTPOINT, 23),

  /* This points to a linked list of post entries, struct curl_httppost */
  CINIT(HTTPPOST, OBJECTPOINT, 24),

  /* name of the file keeping your private SSL-certificate */
  CINIT(SSLCERT, STRINGPOINT, 25),

  /* password for the SSL or SSH private key */
  CINIT(KEYPASSWD, STRINGPOINT, 26),

  /* send TYPE parameter? */
  CINIT(CRLF, LONG, 27),

  /* send linked-list of QUOTE commands */
  CINIT(QUOTE, OBJECTPOINT, 28),

  /* send FILE * or void * to store headers to, if you use a callback it
     is simply passed to the callback unmodified */
  CINIT(HEADERDATA, OBJECTPOINT, 29),

  /* point to a file to read the initial cookies from, also enables
     "cookie awareness" */
  CINIT(COOKIEFILE, STRINGPOINT, 31),

  /* What version to specifically try to use.
     See CURL_SSLVERSION defines below. */
  CINIT(SSLVERSION, LONG, 32),

  /* What kind of HTTP time condition to use, see defines */
  CINIT(TIMECONDITION, LONG, 33),

  /* Time to use with the above condition. Specified in number of seconds
     since 1 Jan 1970 */
  CINIT(TIMEVALUE, LONG, 34),

  /* 35 = OBSOLETE */

  /* Custom request, for customizing the get command like
     HTTP: DELETE, TRACE and others
     FTP: to use a different list command
     */
  CINIT(CUSTOMREQUEST, STRINGPOINT, 36),

  /* FILE handle to use instead of stderr */
  CINIT(STDERR, OBJECTPOINT, 37),

  /* 38 is not used */

  /* send linked-list of post-transfer QUOTE commands */
  CINIT(POSTQUOTE, OBJECTPOINT, 39),

  CINIT(OBSOLETE40, OBJECTPOINT, 40), /* OBSOLETE, do not use! */

  CINIT(VERBOSE, LONG, 41),      /* talk a lot */
  CINIT(HEADER, LONG, 42),       /* throw the header out too */
  CINIT(NOPROGRESS, LONG, 43),   /* shut off the progress meter */
  CINIT(NOBODY, LONG, 44),       /* use HEAD to get http document */
  CINIT(FAILONERROR, LONG, 45),  /* no output on http error codes >= 400 */
  CINIT(UPLOAD, LONG, 46),       /* this is an upload */
  CINIT(POST, LONG, 47),         /* HTTP POST method */
  CINIT(DIRLISTONLY, LONG, 48),  /* bare names when listing directories */

  CINIT(APPEND, LONG, 50),       /* Append instead of overwrite on upload! */

  /* Specify whether to read the user+password from the .netrc or the URL.
   * This must be one of the CURL_NETRC_* enums below. */
  CINIT(NETRC, LONG, 51),

  CINIT(FOLLOWLOCATION, LONG, 52),  /* use Location: Luke! */

  CINIT(TRANSFERTEXT, LONG, 53), /* transfer data in text/ASCII format */
  CINIT(PUT, LONG, 54),          /* HTTP PUT */

  /* 55 = OBSOLETE */

  /* DEPRECATED
   * Function that will be called instead of the internal progress display
   * function. This function should be defined as the curl_progress_callback
   * prototype defines. */
  CINIT(PROGRESSFUNCTION, FUNCTIONPOINT, 56),

  /* Data passed to the CURLOPT_PROGRESSFUNCTION and CURLOPT_XFERINFOFUNCTION
     callbacks */
  CINIT(PROGRESSDATA, OBJECTPOINT, 57),
#define CURLOPT_XFERINFODATA CURLOPT_PROGRESSDATA

  /* We want the referrer field set automatically when following locations */
  CINIT(AUTOREFERER, LONG, 58),

  /* Port of the proxy, can be set in the proxy string as well with:
     "[host]:[port]" */
  CINIT(PROXYPORT, LONG, 59),

  /* size of the POST input data, if strlen() is not good to use */
  CINIT(POSTFIELDSIZE, LONG, 60),

  /* tunnel non-http operations through a HTTP proxy */
  CINIT(HTTPPROXYTUNNEL, LONG, 61),

  /* Set the interface string to use as outgoing network interface */
  CINIT(INTERFACE, STRINGPOINT, 62),

  /* Set the krb4/5 security level, this also enables krb4/5 awareness.  This
   * is a string, 'clear', 'safe', 'confidential' or 'private'.  If the string
   * is set but doesn't match one of these, 'private' will be used.  */
  CINIT(KRBLEVEL, STRINGPOINT, 63),

  /* Set if we should verify the peer in ssl handshake, set 1 to verify. */
  CINIT(SSL_VERIFYPEER, LONG, 64),

  /* The CApath or CAfile used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CINIT(CAINFO, STRINGPOINT, 65),

  /* 66 = OBSOLETE */
  /* 67 = OBSOLETE */

  /* Maximum number of http redirects to follow */
  CINIT(MAXREDIRS, LONG, 68),

  /* Pass a long set to 1 to get the date of the requested document (if
     possible)! Pass a zero to shut it off. */
  CINIT(FILETIME, LONG, 69),

  /* This points to a linked list of telnet options */
  CINIT(TELNETOPTIONS, OBJECTPOINT, 70),

  /* Max amount of cached alive connections */
  CINIT(MAXCONNECTS, LONG, 71),

  CINIT(OBSOLETE72, LONG, 72), /* OBSOLETE, do not use! */

  /* 73 = OBSOLETE */

  /* Set to explicitly use a new connection for the upcoming transfer.
     Do not use this unless you're absolutely sure of this, as it makes the
     operation slower and is less friendly for the network. */
  CINIT(FRESH_CONNECT, LONG, 74),

  /* Set to explicitly forbid the upcoming transfer's connection to be re-used
     when done. Do not use this unless you're absolutely sure of this, as it
     makes the operation slower and is less friendly for the network. */
  CINIT(FORBID_REUSE, LONG, 75),

  /* Set to a file name that contains random data for libcurl to use to
     seed the random engine when doing SSL connects. */
  CINIT(RANDOM_FILE, STRINGPOINT, 76),

  /* Set to the Entropy Gathering Daemon socket pathname */
  CINIT(EGDSOCKET, STRINGPOINT, 77),

  /* Time-out connect operations after this amount of seconds, if connects are
     OK within this time, then fine... This only aborts the connect phase. */
  CINIT(CONNECTTIMEOUT, LONG, 78),

  /* Function that will be called to store headers (instead of fwrite). The
   * parameters will use fwrite() syntax, make sure to follow them. */
  CINIT(HEADERFUNCTION, FUNCTIONPOINT, 79),

  /* Set this to force the HTTP request to get back to GET. Only really usable
     if POST, PUT or a custom request have been used first.
   */
  CINIT(HTTPGET, LONG, 80),

  /* Set if we should verify the Common name from the peer certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches the
   * provided hostname. */
  CINIT(SSL_VERIFYHOST, LONG, 81),

  /* Specify which file name to write all known cookies in after completed
     operation. Set file name to "-" (dash) to make it go to stdout. */
  CINIT(COOKIEJAR, STRINGPOINT, 82),

  /* Specify which SSL ciphers to use */
  CINIT(SSL_CIPHER_LIST, STRINGPOINT, 83),

  /* Specify which HTTP version to use! This must be set to one of the
     CURL_HTTP_VERSION* enums set below. */
  CINIT(HTTP_VERSION, LONG, 84),

  /* Specifically switch on or off the FTP engine's use of the EPSV command. By
     default, that one will always be attempted before the more traditional
     PASV command. */
  CINIT(FTP_USE_EPSV, LONG, 85),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") */
  CINIT(SSLCERTTYPE, STRINGPOINT, 86),

  /* name of the file keeping your private SSL-key */
  CINIT(SSLKEY, STRINGPOINT, 87),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") */
  CINIT(SSLKEYTYPE, STRINGPOINT, 88),

  /* crypto engine for the SSL-sub system */
  CINIT(SSLENGINE, STRINGPOINT, 89),

  /* set the crypto engine for the SSL-sub system as default
     the param has no meaning...
   */
  CINIT(SSLENGINE_DEFAULT, LONG, 90),

  /* Non-zero value means to use the global dns cache */
  CINIT(DNS_USE_GLOBAL_CACHE, LONG, 91), /* DEPRECATED, do not use! */

  /* DNS cache timeout */
  CINIT(DNS_CACHE_TIMEOUT, LONG, 92),

  /* send linked-list of pre-transfer QUOTE commands */
  CINIT(PREQUOTE, OBJECTPOINT, 93),

  /* set the debug function */
  CINIT(DEBUGFUNCTION, FUNCTIONPOINT, 94),

  /* set the data for the debug function */
  CINIT(DEBUGDATA, OBJECTPOINT, 95),

  /* mark this as start of a cookie session */
  CINIT(COOKIESESSION, LONG, 96),

  /* The CApath directory used to validate the peer certificate
     this option is used only if SSL_VERIFYPEER is true */
  CINIT(CAPATH, STRINGPOINT, 97),

  /* Instruct libcurl to use a smaller receive buffer */
  CINIT(BUFFERSIZE, LONG, 98),

  /* Instruct libcurl to not use any signal/alarm handlers, even when using
     timeouts. This option is useful for multi-threaded applications.
     See libcurl-the-guide for more background information. */
  CINIT(NOSIGNAL, LONG, 99),

  /* Provide a CURLShare for mutexing non-ts data */
  CINIT(SHARE, OBJECTPOINT, 100),

  /* indicates type of proxy. accepted values are CURLPROXY_HTTP (default),
     CURLPROXY_HTTPS, CURLPROXY_SOCKS4, CURLPROXY_SOCKS4A and
     CURLPROXY_SOCKS5. */
  CINIT(PROXYTYPE, LONG, 101),

  /* Set the Accept-Encoding string. Use this to tell a server you would like
     the response to be compressed. Before 7.21.6, this was known as
     CURLOPT_ENCODING */
  CINIT(ACCEPT_ENCODING, STRINGPOINT, 102),

  /* Set pointer to private data */
  CINIT(PRIVATE, OBJECTPOINT, 103),

  /* Set aliases for HTTP 200 in the HTTP Response header */
  CINIT(HTTP200ALIASES, OBJECTPOINT, 104),

  /* Continue to send authentication (user+password) when following locations,
     even when hostname changed. This can potentially send off the name
     and password to whatever host the server decides. */
  CINIT(UNRESTRICTED_AUTH, LONG, 105),

  /* Specifically switch on or off the FTP engine's use of the EPRT command (
     it also disables the LPRT attempt). By default, those ones will always be
     attempted before the good old traditional PORT command. */
  CINIT(FTP_USE_EPRT, LONG, 106),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CURLOPT_USERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CINIT(HTTPAUTH, LONG, 107),

  /* Set the ssl context callback function, currently only for OpenSSL ssl_ctx
     in second argument. The function must be matching the
     curl_ssl_ctx_callback proto. */
  CINIT(SSL_CTX_FUNCTION, FUNCTIONPOINT, 108),

  /* Set the userdata for the ssl context callback function's third
     argument */
  CINIT(SSL_CTX_DATA, OBJECTPOINT, 109),

  /* FTP Option that causes missing dirs to be created on the remote server.
     In 7.19.4 we introduced the convenience enums for this option using the
     CURLFTP_CREATE_DIR prefix.
  */
  CINIT(FTP_CREATE_MISSING_DIRS, LONG, 110),

  /* Set this to a bitmask value to enable the particular authentications
     methods you like. Use this in combination with CURLOPT_PROXYUSERPWD.
     Note that setting multiple bits may cause extra network round-trips. */
  CINIT(PROXYAUTH, LONG, 111),

  /* FTP option that changes the timeout, in seconds, associated with
     getting a response.  This is different from transfer timeout time and
     essentially places a demand on the FTP server to acknowledge commands
     in a timely manner. */
  CINIT(FTP_RESPONSE_TIMEOUT, LONG, 112),
#define CURLOPT_SERVER_RESPONSE_TIMEOUT CURLOPT_FTP_RESPONSE_TIMEOUT

  /* Set this option to one of the CURL_IPRESOLVE_* defines (see below) to
     tell libcurl to resolve names to those IP versions only. This only has
     affect on systems with support for more than one, i.e IPv4 _and_ IPv6. */
  CINIT(IPRESOLVE, LONG, 113),

  /* Set this option to limit the size of a file that will be downloaded from
     an HTTP or FTP server.

     Note there is also _LARGE version which adds large file support for
     platforms which have larger off_t sizes.  See MAXFILESIZE_LARGE below. */
  CINIT(MAXFILESIZE, LONG, 114),

  /* See the comment for INFILESIZE above, but in short, specifies
   * the size of the file being uploaded.  -1 means unknown.
   */
  CINIT(INFILESIZE_LARGE, OFF_T, 115),

  /* Sets the continuation offset.  There is also a LONG version of this;
   * look above for RESUME_FROM.
   */
  CINIT(RESUME_FROM_LARGE, OFF_T, 116),

  /* Sets the maximum size of data that will be downloaded from
   * an HTTP or FTP server.  See MAXFILESIZE above for the LONG version.
   */
  CINIT(MAXFILESIZE_LARGE, OFF_T, 117),

  /* Set this option to the file name of your .netrc file you want libcurl
     to parse (using the CURLOPT_NETRC option). If not set, libcurl will do
     a poor attempt to find the user's home directory and check for a .netrc
     file in there. */
  CINIT(NETRC_FILE, STRINGPOINT, 118),

  /* Enable SSL/TLS for FTP, pick one of:
     CURLUSESSL_TRY     - try using SSL, proceed anyway otherwise
     CURLUSESSL_CONTROL - SSL for the control connection or fail
     CURLUSESSL_ALL     - SSL for all communication or fail
  */
  CINIT(USE_SSL, LONG, 119),

  /* The _LARGE version of the standard POSTFIELDSIZE option */
  CINIT(POSTFIELDSIZE_LARGE, OFF_T, 120),

  /* Enable/disable the TCP Nagle algorithm */
  CINIT(TCP_NODELAY, LONG, 121),

  /* 122 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 123 OBSOLETE. Gone in 7.16.0 */
  /* 124 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 125 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 126 OBSOLETE, used in 7.12.3. Gone in 7.13.0 */
  /* 127 OBSOLETE. Gone in 7.16.0 */
  /* 128 OBSOLETE. Gone in 7.16.0 */

  /* When FTP over SSL/TLS is selected (with CURLOPT_USE_SSL), this option
     can be used to change libcurl's default action which is to first try
     "AUTH SSL" and then "AUTH TLS" in this order, and proceed when a OK
     response has been received.

     Available parameters are:
     CURLFTPAUTH_DEFAULT - let libcurl decide
     CURLFTPAUTH_SSL     - try "AUTH SSL" first, then TLS
     CURLFTPAUTH_TLS     - try "AUTH TLS" first, then SSL
  */
  CINIT(FTPSSLAUTH, LONG, 129),

  CINIT(IOCTLFUNCTION, FUNCTIONPOINT, 130),
  CINIT(IOCTLDATA, OBJECTPOINT, 131),

  /* 132 OBSOLETE. Gone in 7.16.0 */
  /* 133 OBSOLETE. Gone in 7.16.0 */

  /* zero terminated string for pass on to the FTP server when asked for
     "account" info */
  CINIT(FTP_ACCOUNT, STRINGPOINT, 134),

  /* feed cookie into cookie engine */
  CINIT(COOKIELIST, STRINGPOINT, 135),

  /* ignore Content-Length */
  CINIT(IGNORE_CONTENT_LENGTH, LONG, 136),

  /* Set to non-zero to skip the IP address received in a 227 PASV FTP server
     response. Typically used for FTP-SSL purposes but is not restricted to
     that. libcurl will then instead use the same IP address it used for the
     control connection. */
  CINIT(FTP_SKIP_PASV_IP, LONG, 137),

  /* Select "file method" to use when doing FTP, see the curl_ftpmethod
     above. */
  CINIT(FTP_FILEMETHOD, LONG, 138),

  /* Local port number to bind the socket to */
  CINIT(LOCALPORT, LONG, 139),

  /* Number of ports to try, including the first one set with LOCALPORT.
     Thus, setting it to 1 will make no additional attempts but the first.
  */
  CINIT(LOCALPORTRANGE, LONG, 140),

  /* no transfer, set up connection and let application use the socket by
     extracting it with CURLINFO_LASTSOCKET */
  CINIT(CONNECT_ONLY, LONG, 141),

  /* Function that will be called to convert from the
     network encoding (instead of using the iconv calls in libcurl) */
  CINIT(CONV_FROM_NETWORK_FUNCTION, FUNCTIONPOINT, 142),

  /* Function that will be called to convert to the
     network encoding (instead of using the iconv calls in libcurl) */
  CINIT(CONV_TO_NETWORK_FUNCTION, FUNCTIONPOINT, 143),

  /* Function that will be called to convert from UTF8
     (instead of using the iconv calls in libcurl)
     Note that this is used only for SSL certificate processing */
  CINIT(CONV_FROM_UTF8_FUNCTION, FUNCTIONPOINT, 144),

  /* if the connection proceeds too quickly then need to slow it down */
  /* limit-rate: maximum number of bytes per second to send or receive */
  CINIT(MAX_SEND_SPEED_LARGE, OFF_T, 145),
  CINIT(MAX_RECV_SPEED_LARGE, OFF_T, 146),

  /* Pointer to command string to send if USER/PASS fails. */
  CINIT(FTP_ALTERNATIVE_TO_USER, STRINGPOINT, 147),

  /* callback function for setting socket options */
  CINIT(SOCKOPTFUNCTION, FUNCTIONPOINT, 148),
  CINIT(SOCKOPTDATA, OBJECTPOINT, 149),

  /* set to 0 to disable session ID re-use for this transfer, default is
     enabled (== 1) */
  CINIT(SSL_SESSIONID_CACHE, LONG, 150),

  /* allowed SSH authentication methods */
  CINIT(SSH_AUTH_TYPES, LONG, 151),

  /* Used by scp/sftp to do public/private key authentication */
  CINIT(SSH_PUBLIC_KEYFILE, STRINGPOINT, 152),
  CINIT(SSH_PRIVATE_KEYFILE, STRINGPOINT, 153),

  /* Send CCC (Clear Command Channel) after authentication */
  CINIT(FTP_SSL_CCC, LONG, 154),

  /* Same as TIMEOUT and CONNECTTIMEOUT, but with ms resolution */
  CINIT(TIMEOUT_MS, LONG, 155),
  CINIT(CONNECTTIMEOUT_MS, LONG, 156),

  /* set to zero to disable the libcurl's decoding and thus pass the raw body
     data to the application even when it is encoded/compressed */
  CINIT(HTTP_TRANSFER_DECODING, LONG, 157),
  CINIT(HTTP_CONTENT_DECODING, LONG, 158),

  /* Permission used when creating new files and directories on the remote
     server for protocols that support it, SFTP/SCP/FILE */
  CINIT(NEW_FILE_PERMS, LONG, 159),
  CINIT(NEW_DIRECTORY_PERMS, LONG, 160),

  /* Set the behaviour of POST when redirecting. Values must be set to one
     of CURL_REDIR* defines below. This used to be called CURLOPT_POST301 */
  CINIT(POSTREDIR, LONG, 161),

  /* used by scp/sftp to verify the host's public key */
  CINIT(SSH_HOST_PUBLIC_KEY_MD5, STRINGPOINT, 162),

  /* Callback function for opening socket (instead of socket(2)). Optionally,
     callback is able change the address or refuse to connect returning
     CURL_SOCKET_BAD.  The callback should have type
     curl_opensocket_callback */
  CINIT(OPENSOCKETFUNCTION, FUNCTIONPOINT, 163),
  CINIT(OPENSOCKETDATA, OBJECTPOINT, 164),

  /* POST volatile input fields. */
  CINIT(COPYPOSTFIELDS, OBJECTPOINT, 165),

  /* set transfer mode (;type=<a|i>) when doing FTP via an HTTP proxy */
  CINIT(PROXY_TRANSFER_MODE, LONG, 166),

  /* Callback function for seeking in the input stream */
  CINIT(SEEKFUNCTION, FUNCTIONPOINT, 167),
  CINIT(SEEKDATA, OBJECTPOINT, 168),

  /* CRL file */
  CINIT(CRLFILE, STRINGPOINT, 169),

  /* Issuer certificate */
  CINIT(ISSUERCERT, STRINGPOINT, 170),

  /* (IPv6) Address scope */
  CINIT(ADDRESS_SCOPE, LONG, 171),

  /* Collect certificate chain info and allow it to get retrievable with
     CURLINFO_CERTINFO after the transfer is complete. */
  CINIT(CERTINFO, LONG, 172),

  /* "name" and "pwd" to use when fetching. */
  CINIT(USERNAME, STRINGPOINT, 173),
  CINIT(PASSWORD, STRINGPOINT, 174),

    /* "name" and "pwd" to use with Proxy when fetching. */
  CINIT(PROXYUSERNAME, STRINGPOINT, 175),
  CINIT(PROXYPASSWORD, STRINGPOINT, 176),

  /* Comma separated list of hostnames defining no-proxy zones. These should
     match both hostnames directly, and hostnames within a domain. For
     example, local.com will match local.com and www.local.com, but NOT
     notlocal.com or www.notlocal.com. For compatibility with other
     implementations of this, .local.com will be considered to be the same as
     local.com. A single * is the only valid wildcard, and effectively
     disables the use of proxy. */
  CINIT(NOPROXY, STRINGPOINT, 177),

  /* block size for TFTP transfers */
  CINIT(TFTP_BLKSIZE, LONG, 178),

  /* Socks Service */
  CINIT(SOCKS5_GSSAPI_SERVICE, STRINGPOINT, 179), /* DEPRECATED, do not use! */

  /* Socks Service */
  CINIT(SOCKS5_GSSAPI_NEC, LONG, 180),

  /* set the bitmask for the protocols that are allowed to be used for the
     transfer, which thus helps the app which takes URLs from users or other
     external inputs and want to restrict what protocol(s) to deal
     with. Defaults to CURLPROTO_ALL. */
  CINIT(PROTOCOLS, LONG, 181),

  /* set the bitmask for the protocols that libcurl is allowed to follow to,
     as a subset of the CURLOPT_PROTOCOLS ones. That means the protocol needs
     to be set in both bitmasks to be allowed to get redirected to. Defaults
     to all protocols except FILE and SCP. */
  CINIT(REDIR_PROTOCOLS, LONG, 182),

  /* set the SSH knownhost file name to use */
  CINIT(SSH_KNOWNHOSTS, STRINGPOINT, 183),

  /* set the SSH host key callback, must point to a curl_sshkeycallback
     function */
  CINIT(SSH_KEYFUNCTION, FUNCTIONPOINT, 184),

  /* set the SSH host key callback custom pointer */
  CINIT(SSH_KEYDATA, OBJECTPOINT, 185),

  /* set the SMTP mail originator */
  CINIT(MAIL_FROM, STRINGPOINT, 186),

  /* set the list of SMTP mail receiver(s) */
  CINIT(MAIL_RCPT, OBJECTPOINT, 187),

  /* FTP: send PRET before PASV */
  CINIT(FTP_USE_PRET, LONG, 188),

  /* RTSP request method (OPTIONS, SETUP, PLAY, etc...) */
  CINIT(RTSP_REQUEST, LONG, 189),

  /* The RTSP session identifier */
  CINIT(RTSP_SESSION_ID, STRINGPOINT, 190),

  /* The RTSP stream URI */
  CINIT(RTSP_STREAM_URI, STRINGPOINT, 191),

  /* The Transport: header to use in RTSP requests */
  CINIT(RTSP_TRANSPORT, STRINGPOINT, 192),

  /* Manually initialize the client RTSP CSeq for this handle */
  CINIT(RTSP_CLIENT_CSEQ, LONG, 193),

  /* Manually initialize the server RTSP CSeq for this handle */
  CINIT(RTSP_SERVER_CSEQ, LONG, 194),

  /* The stream to pass to INTERLEAVEFUNCTION. */
  CINIT(INTERLEAVEDATA, OBJECTPOINT, 195),

  /* Let the application define a custom write method for RTP data */
  CINIT(INTERLEAVEFUNCTION, FUNCTIONPOINT, 196),

  /* Turn on wildcard matching */
  CINIT(WILDCARDMATCH, LONG, 197),

  /* Directory matching callback called before downloading of an
     individual file (chunk) started */
  CINIT(CHUNK_BGN_FUNCTION, FUNCTIONPOINT, 198),

  /* Directory matching callback called after the file (chunk)
     was downloaded, or skipped */
  CINIT(CHUNK_END_FUNCTION, FUNCTIONPOINT, 199),

  /* Change match (fnmatch-like) callback for wildcard matching */
  CINIT(FNMATCH_FUNCTION, FUNCTIONPOINT, 200),

  /* Let the application define custom chunk data pointer */
  CINIT(CHUNK_DATA, OBJECTPOINT, 201),

  /* FNMATCH_FUNCTION user pointer */
  CINIT(FNMATCH_DATA, OBJECTPOINT, 202),

  /* send linked-list of name:port:address sets */
  CINIT(RESOLVE, OBJECTPOINT, 203),

  /* Set a username for authenticated TLS */
  CINIT(TLSAUTH_USERNAME, STRINGPOINT, 204),

  /* Set a password for authenticated TLS */
  CINIT(TLSAUTH_PASSWORD, STRINGPOINT, 205),

  /* Set authentication type for authenticated TLS */
  CINIT(TLSAUTH_TYPE, STRINGPOINT, 206),

  /* Set to 1 to enable the "TE:" header in HTTP requests to ask for
     compressed transfer-encoded responses. Set to 0 to disable the use of TE:
     in outgoing requests. The current default is 0, but it might change in a
     future libcurl release.

     libcurl will ask for the compressed methods it knows of, and if that
     isn't any, it will not ask for transfer-encoding at all even if this
     option is set to 1.

  */
  CINIT(TRANSFER_ENCODING, LONG, 207),

  /* Callback function for closing socket (instead of close(2)). The callback
     should have type curl_closesocket_callback */
  CINIT(CLOSESOCKETFUNCTION, FUNCTIONPOINT, 208),
  CINIT(CLOSESOCKETDATA, OBJECTPOINT, 209),

  /* allow GSSAPI credential delegation */
  CINIT(GSSAPI_DELEGATION, LONG, 210),

  /* Set the name servers to use for DNS resolution */
  CINIT(DNS_SERVERS, STRINGPOINT, 211),

  /* Time-out accept operations (currently for FTP only) after this amount
     of milliseconds. */
  CINIT(ACCEPTTIMEOUT_MS, LONG, 212),

  /* Set TCP keepalive */
  CINIT(TCP_KEEPALIVE, LONG, 213),

  /* non-universal keepalive knobs (Linux, AIX, HP-UX, more) */
  CINIT(TCP_KEEPIDLE, LONG, 214),
  CINIT(TCP_KEEPINTVL, LONG, 215),

  /* Enable/disable specific SSL features with a bitmask, see CURLSSLOPT_* */
  CINIT(SSL_OPTIONS, LONG, 216),

  /* Set the SMTP auth originator */
  CINIT(MAIL_AUTH, STRINGPOINT, 217),

  /* Enable/disable SASL initial response */
  CINIT(SASL_IR, LONG, 218),

  /* Function that will be called instead of the internal progress display
   * function. This function should be defined as the curl_xferinfo_callback
   * prototype defines. (Deprecates CURLOPT_PROGRESSFUNCTION) */
  CINIT(XFERINFOFUNCTION, FUNCTIONPOINT, 219),

  /* The XOAUTH2 bearer token */
  CINIT(XOAUTH2_BEARER, STRINGPOINT, 220),

  /* Set the interface string to use as outgoing network
   * interface for DNS requests.
   * Only supported by the c-ares DNS backend */
  CINIT(DNS_INTERFACE, STRINGPOINT, 221),

  /* Set the local IPv4 address to use for outgoing DNS requests.
   * Only supported by the c-ares DNS backend */
  CINIT(DNS_LOCAL_IP4, STRINGPOINT, 222),

  /* Set the local IPv4 address to use for outgoing DNS requests.
   * Only supported by the c-ares DNS backend */
  CINIT(DNS_LOCAL_IP6, STRINGPOINT, 223),

  /* Set authentication options directly */
  CINIT(LOGIN_OPTIONS, STRINGPOINT, 224),

  /* Enable/disable TLS NPN extension (http2 over ssl might fail without) */
  CINIT(SSL_ENABLE_NPN, LONG, 225),

  /* Enable/disable TLS ALPN extension (http2 over ssl might fail without) */
  CINIT(SSL_ENABLE_ALPN, LONG, 226),

  /* Time to wait for a response to a HTTP request containing an
   * Expect: 100-continue header before sending the data anyway. */
  CINIT(EXPECT_100_TIMEOUT_MS, LONG, 227),

  /* This points to a linked list of headers used for proxy requests only,
     struct curl_slist kind */
  CINIT(PROXYHEADER, OBJECTPOINT, 228),

  /* Pass in a bitmask of "header options" */
  CINIT(HEADEROPT, LONG, 229),

  /* The public key in DER form used to validate the peer public key
     this option is used only if SSL_VERIFYPEER is true */
  CINIT(PINNEDPUBLICKEY, STRINGPOINT, 230),

  /* Path to Unix domain socket */
  CINIT(UNIX_SOCKET_PATH, STRINGPOINT, 231),

  /* Set if we should verify the certificate status. */
  CINIT(SSL_VERIFYSTATUS, LONG, 232),

  /* Set if we should enable TLS false start. */
  CINIT(SSL_FALSESTART, LONG, 233),

  /* Do not squash dot-dot sequences */
  CINIT(PATH_AS_IS, LONG, 234),

  /* Proxy Service Name */
  CINIT(PROXY_SERVICE_NAME, STRINGPOINT, 235),

  /* Service Name */
  CINIT(SERVICE_NAME, STRINGPOINT, 236),

  /* Wait/don't wait for pipe/mutex to clarify */
  CINIT(PIPEWAIT, LONG, 237),

  /* Set the protocol used when curl is given a URL without a protocol */
  CINIT(DEFAULT_PROTOCOL, STRINGPOINT, 238),

  /* Set stream weight, 1 - 256 (default is 16) */
  CINIT(STREAM_WEIGHT, LONG, 239),

  /* Set stream dependency on another CURL handle */
  CINIT(STREAM_DEPENDS, OBJECTPOINT, 240),

  /* Set E-xclusive stream dependency on another CURL handle */
  CINIT(STREAM_DEPENDS_E, OBJECTPOINT, 241),

  /* Do not send any tftp option requests to the server */
  CINIT(TFTP_NO_OPTIONS, LONG, 242),

  /* Linked-list of host:port:connect-to-host:connect-to-port,
     overrides the URL's host:port (only for the network layer) */
  CINIT(CONNECT_TO, OBJECTPOINT, 243),

  /* Set TCP Fast Open */
  CINIT(TCP_FASTOPEN, LONG, 244),

  /* Continue to send data if the server responds early with an
   * HTTP status code >= 300 */
  CINIT(KEEP_SENDING_ON_ERROR, LONG, 245),

  /* The CApath or CAfile used to validate the proxy certificate
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CINIT(PROXY_CAINFO, STRINGPOINT, 246),

  /* The CApath directory used to validate the proxy certificate
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CINIT(PROXY_CAPATH, STRINGPOINT, 247),

  /* Set if we should verify the proxy in ssl handshake,
     set 1 to verify. */
  CINIT(PROXY_SSL_VERIFYPEER, LONG, 248),

  /* Set if we should verify the Common name from the proxy certificate in ssl
   * handshake, set 1 to check existence, 2 to ensure that it matches
   * the provided hostname. */
  CINIT(PROXY_SSL_VERIFYHOST, LONG, 249),

  /* What version to specifically try to use for proxy.
     See CURL_SSLVERSION defines below. */
  CINIT(PROXY_SSLVERSION, LONG, 250),

  /* Set a username for authenticated TLS for proxy */
  CINIT(PROXY_TLSAUTH_USERNAME, STRINGPOINT, 251),

  /* Set a password for authenticated TLS for proxy */
  CINIT(PROXY_TLSAUTH_PASSWORD, STRINGPOINT, 252),

  /* Set authentication type for authenticated TLS for proxy */
  CINIT(PROXY_TLSAUTH_TYPE, STRINGPOINT, 253),

  /* name of the file keeping your private SSL-certificate for proxy */
  CINIT(PROXY_SSLCERT, STRINGPOINT, 254),

  /* type of the file keeping your SSL-certificate ("DER", "PEM", "ENG") for
     proxy */
  CINIT(PROXY_SSLCERTTYPE, STRINGPOINT, 255),

  /* name of the file keeping your private SSL-key for proxy */
  CINIT(PROXY_SSLKEY, STRINGPOINT, 256),

  /* type of the file keeping your private SSL-key ("DER", "PEM", "ENG") for
     proxy */
  CINIT(PROXY_SSLKEYTYPE, STRINGPOINT, 257),

  /* password for the SSL private key for proxy */
  CINIT(PROXY_KEYPASSWD, STRINGPOINT, 258),

  /* Specify which SSL ciphers to use for proxy */
  CINIT(PROXY_SSL_CIPHER_LIST, STRINGPOINT, 259),

  /* CRL file for proxy */
  CINIT(PROXY_CRLFILE, STRINGPOINT, 260),

  /* Enable/disable specific SSL features with a bitmask for proxy, see
     CURLSSLOPT_* */
  CINIT(PROXY_SSL_OPTIONS, LONG, 261),

  /* Name of pre proxy to use. */
  CINIT(PRE_PROXY, STRINGPOINT, 262),

  /* The public key in DER form used to validate the proxy public key
     this option is used only if PROXY_SSL_VERIFYPEER is true */
  CINIT(PROXY_PINNEDPUBLICKEY, STRINGPOINT, 263),

  /* Path to an abstract Unix domain socket */
  CINIT(ABSTRACT_UNIX_SOCKET, STRINGPOINT, 264),

  /* Suppress proxy CONNECT response headers from user callbacks */
  CINIT(SUPPRESS_CONNECT_HEADERS, LONG, 265),

  /* The request target, instead of extracted from the URL */
  CINIT(REQUEST_TARGET, STRINGPOINT, 266),

  /* bitmask of allowed auth methods for connections to SOCKS5 proxies */
  CINIT(SOCKS5_AUTH, LONG, 267),

  CURLOPT_LASTENTRY /* the last unused */
} CURLoption;

/* Provide defines for really old option names */
#define CURLOPT_FILE CURLOPT_WRITEDATA /* name changed in 7.9.7 */
#define CURLOPT_INFILE CURLOPT_READDATA /* name changed in 7.9.7 */
#define CURLOPT_WRITEHEADER CURLOPT_HEADERDATA

//***************************************************************************
// Copy of curl include files - Multi interface
//***************************************************************************

typedef void CURLM;

typedef enum {
  CURLM_CALL_MULTI_PERFORM = -1, /* please call curl_multi_perform() or
                                    curl_multi_socket*() soon */
  CURLM_OK,
  CURLM_BAD_HANDLE,      /* the passed-in handle is not a valid CURLM handle */
  CURLM_BAD_EASY_HANDLE, /* an easy handle was not good/valid */
  CURLM_OUT_OF_MEMORY,   /* if you ever get this, you're in deep sh*t */
  CURLM_INTERNAL_ERROR,  /* this is a libcurl bug */
  CURLM_BAD_SOCKET,      /* the passed in socket argument did not match */
  CURLM_UNKNOWN_OPTION,  /* curl_multi_setopt() with unsupported option */
  CURLM_ADDED_ALREADY,   /* an easy handle already added to a multi handle was
                            attempted to get added - again */
  CURLM_LAST
} CURLMcode;

//***************************************************************************
// Copy of curl include files - Version interface
//***************************************************************************

typedef enum {
  CURLVERSION_FIRST,
  CURLVERSION_SECOND,
  CURLVERSION_THIRD,
  CURLVERSION_FOURTH,
  CURLVERSION_LAST /* never actually use this */
} CURLversion;

/* The 'CURLVERSION_NOW' is the symbolic name meant to be used by
   basically all programs ever that want to get version information. It is
   meant to be a built-in version number for what kind of struct the caller
   expects. If the struct ever changes, we redefine the NOW to another enum
   from above. */
#define CURLVERSION_NOW CURLVERSION_FOURTH

typedef struct {
  CURLversion age;          /* age of the returned struct */
  const char *version;      /* LIBCURL_VERSION */
  unsigned int version_num; /* LIBCURL_VERSION_NUM */
  const char *host;         /* OS/host/cpu/machine when configured */
  int features;             /* bitmask, see defines below */
  const char *ssl_version;  /* human readable string */
  long ssl_version_num;     /* not used anymore, always 0 */
  const char *libz_version; /* human readable string */
  /* protocols is terminated by an entry with a NULL protoname */
  const char * const *protocols;

  /* The fields below this were added in CURLVERSION_SECOND */
  const char *ares;
  int ares_num;

  /* This field was added in CURLVERSION_THIRD */
  const char *libidn;

  /* These field were added in CURLVERSION_FOURTH */

  /* Same as '_libiconv_version' if built with HAVE_ICONV */
  int iconv_ver_num;

  const char *libssh_version; /* human readable string */

} curl_version_info_data;


#define CURL_ERROR_SIZE 256

//***************************************************************************
// Dynamic load stuff
//***************************************************************************

extern "C"
{

#ifdef MEDIAINFO_GLIBC
    #include <gmodule.h>
    static GModule* libcurl_Module=NULL;
#elif defined (_WIN32) || defined (WIN32)
    #undef __TEXT
    #include <windows.h>
    static HMODULE  libcurl_Module=NULL;
#else
    #include <dlfcn.h>
    static void*    libcurl_Module=NULL;
#endif

size_t libcurl_Module_Count=0;

#ifdef MEDIAINFO_GLIBC
#define MEDIAINFO_ASSIGN(_Name,_Name2) \
    if (!g_module_symbol (libcurl_Module, _Name2, (gpointer*)&_Name)) \
        Errors++;
#elif defined (_WIN32) || defined (WIN32)
#define MEDIAINFO_ASSIGN(_Name,_Name2) \
    _Name=(LIBCURL_##_Name)GetProcAddress(libcurl_Module, _Name2); \
    if (_Name==NULL) Errors++;
#else
#define MEDIAINFO_ASSIGN(_Name,_Name2) \
    _Name=(LIBCURL_##_Name)dlsym(libcurl_Module, _Name2); \
    if (_Name==NULL) Errors++;
#endif

//---------------------------------------------------------------------------
// Easy interface
typedef CURL* (*LIBCURL_curl_easy_init)   ();   static LIBCURL_curl_easy_init    curl_easy_init;
typedef CURLcode (*LIBCURL_curl_easy_setopt) (CURL *curl, CURLoption option, ...);   static LIBCURL_curl_easy_setopt  curl_easy_setopt;
typedef CURLcode (*LIBCURL_curl_easy_perform)(CURL *curl);   static LIBCURL_curl_easy_perform curl_easy_perform;
typedef void (*LIBCURL_curl_easy_cleanup)(CURL *curl);   static LIBCURL_curl_easy_cleanup curl_easy_cleanup;
typedef CURLcode (*LIBCURL_curl_easy_getinfo)(CURL *curl, CURLINFO info, ...);   static LIBCURL_curl_easy_getinfo curl_easy_getinfo;
typedef struct curl_slist* (*LIBCURL_curl_slist_append)   (struct curl_slist *, const char *);   static LIBCURL_curl_slist_append    curl_slist_append;
typedef void (*LIBCURL_curl_slist_free_all)   (struct curl_slist *);   static LIBCURL_curl_slist_free_all    curl_slist_free_all;
typedef CURL* (*LIBCURL_curl_easy_duphandle)(CURL *curl);   static LIBCURL_curl_easy_duphandle curl_easy_duphandle;
typedef const char* (*LIBCURL_curl_easy_strerror)(CURLcode curlcode);   static LIBCURL_curl_easy_strerror curl_easy_strerror;
typedef curl_version_info_data* (*LIBCURL_curl_version_info)(CURLversion version);   static LIBCURL_curl_version_info curl_version_info;

//---------------------------------------------------------------------------
// Multi interface
typedef CURLM* (*LIBCURL_curl_multi_init)   ();   static LIBCURL_curl_multi_init    curl_multi_init;
typedef CURLMcode (*LIBCURL_curl_multi_add_handle) (CURLM *multi_handle, CURL *curl_handle);   static LIBCURL_curl_multi_add_handle curl_multi_add_handle;
typedef CURLMcode (*LIBCURL_curl_multi_remove_handle) (CURLM *multi_handle, CURL *curl_handle);   static LIBCURL_curl_multi_remove_handle curl_multi_remove_handle;
typedef CURLMcode (*LIBCURL_curl_multi_perform)(CURLM *curl, int *running_handles);   static LIBCURL_curl_multi_perform curl_multi_perform;
typedef void (*LIBCURL_curl_multi_cleanup)(CURLM *curl);   static LIBCURL_curl_multi_cleanup curl_multi_cleanup;

}

#endif
