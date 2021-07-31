//
// papplMainloop support functions for the Printer Application Framework
//
// Copyright © 2020 by Michael R Sweet.
//
// Licensed under Apache License v2.0.  See the file "LICENSE" for more
// information.
//

//
// Include necessary headers
//

#  include "pappl-private.h"
#  if _WIN32
#  else
#    include <spawn.h>
#    include <libgen.h>
#  endif // _WIN32


//
// Globals...
//

char	*_papplMainloopPath = NULL;	// Path to self


//
// Local functions
//

static int	get_length(const char *value);


//
// '_papplMainloopAddOptions()' - Add default/job template attributes from options.
//

void
_papplMainloopAddOptions(
    ipp_t         *request,		// I - IPP request
    int           num_options,		// I - Number of options
    cups_option_t *options,		// I - Options
    ipp_t         *supported)		// I - Supported attributes
{
  ipp_attribute_t *job_attrs;		// job-creation-attributes-supported
  int		is_default;		// Adding xxx-default attributes?
  ipp_tag_t	group_tag;		// Group to add to
  char		*end;			// End of value
  const char	*value;			// String value
  int		intvalue;		// Integer value
  const char	*media_left_offset = cupsGetOption("media-left-offset", num_options, options),
		*media_source = cupsGetOption("media-source", num_options, options),
                *media_top_offset = cupsGetOption("media-top-offset", num_options, options),
		*media_tracking = cupsGetOption("media-tracking", num_options, options),
		*media_type = cupsGetOption("media-type", num_options, options);
					// media-xxx member values


  // Determine what kind of options we are adding...
  group_tag  = ippGetOperation(request) == IPP_PRINT_JOB ? IPP_TAG_JOB : IPP_TAG_PRINTER;
  is_default = (group_tag == IPP_TAG_PRINTER);

  if (is_default)
  {
    // Add Printer Description attributes...
    if ((value = cupsGetOption("label-mode-configured", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "label-mode-configured", NULL, value);

    if ((value = cupsGetOption("label-tear-offset-configured", num_options, options)) != NULL)
      ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "label-tear-offset-configured", get_length(value));

    if ((value = cupsGetOption("media-ready", num_options, options)) != NULL)
    {
      int	num_values;		// Number of values
      char	*values[PAPPL_MAX_SOURCE],
					// Pointers to size strings
		*cvalue,		// Copied value
		*cptr;			// Pointer into copy

      num_values = 0;
      cvalue     = strdup(value);
      cptr       = cvalue;

#if _WIN32
      if ((values[num_values] = strtok_s(cvalue, ",", &cptr)) != NULL)
      {
        num_values ++;

        while (num_values < PAPPL_MAX_SOURCE && (values[num_values] = strtok_s(NULL, ",", &cptr)) != NULL)
          num_values ++;
      }
#else
      while (num_values < PAPPL_MAX_SOURCE && (values[num_values] = strsep(&cptr, ",")) != NULL)
        num_values ++;
#endif // _WIN32

      if (num_values > 0)
        ippAddStrings(request, IPP_TAG_PRINTER, IPP_TAG_KEYWORD, "media-ready", num_values, NULL, (const char * const *)values);

      free(cvalue);
    }

    if ((value = cupsGetOption("printer-darkness-configured", num_options, options)) != NULL)
    {
      if ((intvalue = (int)strtol(value, &end, 10)) >= 0 && intvalue <= 100 && errno != ERANGE && !*end)
        ippAddInteger(request, IPP_TAG_PRINTER, IPP_TAG_INTEGER, "printer-darkness-configured", intvalue);
    }

    if ((value = cupsGetOption("printer-geo-location", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_URI, "printer-geo-location", NULL, value);

    if ((value = cupsGetOption("printer-location", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-location", NULL, value);

    if ((value = cupsGetOption("printer-organization", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-organization", NULL, value);

    if ((value = cupsGetOption("printer-organizational-unit", num_options, options)) != NULL)
      ippAddString(request, IPP_TAG_PRINTER, IPP_TAG_TEXT, "printer-organizational-unit", NULL, value);
  }

  if ((value = cupsGetOption("copies", num_options, options)) == NULL)
    value = cupsGetOption("copies-default", num_options, options);
  if (value && (intvalue = (int)strtol(value, &end, 10)) >= 1 && intvalue <= 9999 && errno != ERANGE && !*end)
    ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? "copies-default" : "copies", intvalue);

  value = cupsGetOption("media", num_options, options);
  if (media_left_offset || media_source || media_top_offset || media_tracking || media_type)
  {
    // Add media-col
    ipp_t 	*media_col = ippNew();	// media-col value
    pwg_media_t *pwg = pwgMediaForPWG(value);
					// Size

    if (pwg)
    {
      ipp_t *media_size = ippNew();	// media-size value

      ippAddInteger(media_size, IPP_TAG_ZERO, IPP_TAG_INTEGER, "x-dimension", pwg->width);
      ippAddInteger(media_size, IPP_TAG_ZERO, IPP_TAG_INTEGER, "y-dimension", pwg->length);
      ippAddCollection(media_col, IPP_TAG_ZERO, "media-size", media_size);
      ippDelete(media_size);
    }

    if (media_left_offset)
      ippAddInteger(media_col, IPP_TAG_ZERO, IPP_TAG_INTEGER, "media-left-offset", get_length(media_left_offset));

    if (media_source)
      ippAddString(media_col, IPP_TAG_ZERO, IPP_TAG_KEYWORD, "media-source", NULL, media_source);

    if (media_top_offset)
      ippAddInteger(media_col, IPP_TAG_ZERO, IPP_TAG_INTEGER, "media-top-offset", get_length(media_top_offset));

    if (media_tracking)
      ippAddString(media_col, IPP_TAG_ZERO, IPP_TAG_KEYWORD, "media-tracking", NULL, media_tracking);

    if (media_type)
      ippAddString(media_col, IPP_TAG_ZERO, IPP_TAG_KEYWORD, "media-type", NULL, media_type);

    ippAddCollection(request, group_tag, is_default ? "media-col-default" : "media-col", media_col);
    ippDelete(media_col);
  }
  else if (value)
  {
    // Add media
    ippAddString(request, IPP_TAG_JOB, IPP_TAG_KEYWORD, is_default ? "media-default" : "media", NULL, value);
  }

  if ((value = cupsGetOption("orientation-requested", num_options, options)) == NULL)
    value = cupsGetOption("orientation-requested-default", num_options, options);
  if (value)
  {
    if ((intvalue = ippEnumValue("orientation-requested", value)) != 0)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "orientation-requested-default" : "orientation-requested", intvalue);
    else if ((intvalue = (int)strtol(value, &end, 10)) >= IPP_ORIENT_PORTRAIT && intvalue <= IPP_ORIENT_NONE && errno != ERANGE && !*end)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "orientation-requested-default" : "orientation-requested", intvalue);
  }

  if ((value = cupsGetOption("print-color-mode", num_options, options)) == NULL)
    value = cupsGetOption("print-color-mode-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "print-color-mode-default" : "print-color-mode", NULL, value);

  if ((value = cupsGetOption("print-content-optimize", num_options, options)) == NULL)
    value = cupsGetOption("print-content-optimize-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "print-content-optimize-mode-default" : "print-content-optimize", NULL, value);

  if ((value = cupsGetOption("print-darkness", num_options, options)) == NULL)
    value = cupsGetOption("print-darkness-default", num_options, options);
  if (value && (intvalue = (int)strtol(value, &end, 10)) >= -100 && intvalue <= 100 && errno != ERANGE && !*end)
    ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? "print-darkness-default" : "print-darkness", intvalue);

  if ((value = cupsGetOption("print-quality", num_options, options)) == NULL)
    value = cupsGetOption("print-quality-default", num_options, options);
  if (value)
  {
    if ((intvalue = ippEnumValue("print-quality", value)) != 0)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "print-quality-default" : "print-quality", intvalue);
    else if ((intvalue = (int)strtol(value, &end, 10)) >= IPP_QUALITY_DRAFT && intvalue <= IPP_QUALITY_HIGH && errno != ERANGE && !*end)
      ippAddInteger(request, group_tag, IPP_TAG_ENUM, is_default ? "print-quality-default" : "print-quality", intvalue);
  }

  if ((value = cupsGetOption("print-scaling", num_options, options)) == NULL)
    value = cupsGetOption("print-scaling-default", num_options, options);
  if (value)
    ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? "print-scaling-default" : "print-scaling", NULL, value);

  if ((value = cupsGetOption("print-speed", num_options, options)) == NULL)
    value = cupsGetOption("print-speed-default", num_options, options);
  if (value)
    ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? "print-speed-default" : "print-speed", get_length(value));

  if ((value = cupsGetOption("printer-resolution", num_options, options)) == NULL)
    value = cupsGetOption("printer-resolution-default", num_options, options);

  if (value)
  {
    int		xres, yres;		// Resolution values
    char	units[32];		// Resolution units

    if (sscanf(value, "%dx%d%31s", &xres, &yres, units) != 3)
    {
      if (sscanf(value, "%d%31s", &xres, units) != 2)
      {
        xres = 300;

        strlcpy(units, "dpi", sizeof(units));
      }

      yres = xres;
    }

    ippAddResolution(request, group_tag, is_default ? "printer-resolution-default" : "printer-resolution", !strcmp(units, "dpi") ? IPP_RES_PER_INCH : IPP_RES_PER_CM, xres, yres);
  }

  // Vendor attributes/options
  if ((job_attrs = ippFindAttribute(supported, "job-creation-attributes-supported", IPP_TAG_KEYWORD)) != NULL)
  {
    int		i,			// Looping var
		count;			// Count
    const char	*name;			// Attribute name
    char	defname[128],		// xxx-default name
		supname[128];		// xxx-supported name
    ipp_attribute_t *attr;		// Attribute

    for (i = 0, count = ippGetCount(job_attrs); i < count; i ++)
    {
      name = ippGetString(job_attrs, i, NULL);

      snprintf(defname, sizeof(defname), "%s-default", name);
      snprintf(supname, sizeof(supname), "%s-supported", name);

      if ((value = cupsGetOption(name, num_options, options)) == NULL)
        value = cupsGetOption(defname, num_options, options);

      if (!value)
        continue;

      if (!strcmp(name, "copies") || !strcmp(name, "finishings") || !strcmp(name, "media") || !strcmp(name, "orientation-requested") || !strcmp(name, "print-color-mode") || !strcmp(name, "print-content-optimize") || !strcmp(name, "print-darkness") || !strcmp(name, "print-quality") || !strcmp(name, "print-scaling") || !strcmp(name, "print-speed") || !strcmp(name, "printer-resolution"))
        continue;

      if ((attr = ippFindAttribute(supported, supname, IPP_TAG_ZERO)) != NULL)
      {
	switch (ippGetValueTag(attr))
	{
	  case IPP_TAG_BOOLEAN :
	      ippAddBoolean(request, group_tag, is_default ? defname : name, !strcmp(value, "true"));
	      break;

	  case IPP_TAG_INTEGER :
	  case IPP_TAG_RANGE :
	      intvalue = (int)strtol(value, &end, 10);
	      if (errno != ERANGE && !*end)
	        ippAddInteger(request, group_tag, IPP_TAG_INTEGER, is_default ? defname : name, intvalue);
	      break;

	  case IPP_TAG_KEYWORD :
	      ippAddString(request, group_tag, IPP_TAG_KEYWORD, is_default ? defname : name, NULL, value);
	      break;

	  default :
	      break;
	}
      }
      else
      {
	ippAddString(request, group_tag, IPP_TAG_TEXT, is_default ? defname : name, NULL, value);
      }
    }
  }
}


//
// '_papplMainloopAddPrinterURI()' - Add the printer-uri attribute and return a
//                                   resource path.
//

void
_papplMainloopAddPrinterURI(
    ipp_t      *request,		// I - IPP request
    const char *printer_name,		// I - Printer name
    char       *resource,		// I - Resource path buffer
    size_t     rsize)			// I - Size of buffer
{
  char	uri[1024];			// printer-uri value


  snprintf(resource, rsize, "/ipp/print/%s", printer_name);
  httpAssembleURI(HTTP_URI_CODING_ALL, uri, sizeof(uri), "ipp", NULL, "localhost", 0, resource);

  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, uri);
}


//
// '_papplMainloopConnect()' - Connect to the local server.
//

http_t *				// O - HTTP connection
_papplMainloopConnect(
    const char *base_name,		// I - Printer application name
    bool       auto_start)		// I - `true` to start server if not running
{
  http_t	*http;			// HTTP connection
  char		sockname[1024];		// Socket filename


  // See if the server is running...
#if _WIN32
  http = httpConnect2(_papplMainloopGetServerPath(base_name, 0, sockname, sizeof(sockname)), 0, NULL, AF_UNSPEC, HTTP_ENCRYPTION_IF_REQUESTED, 1, 30000, NULL);
#else
  http = httpConnect2(_papplMainloopGetServerPath(base_name, getuid(), sockname, sizeof(sockname)), 0, NULL, AF_UNSPEC, HTTP_ENCRYPTION_IF_REQUESTED, 1, 30000, NULL);

  if (!http && getuid())
  {
    // Try root server...
    http = httpConnect2(_papplMainloopGetServerPath(base_name, 0, sockname, sizeof(sockname)), 0, NULL, AF_UNSPEC, HTTP_ENCRYPTION_IF_REQUESTED, 1, 30000, NULL);
  }

  if (!http && auto_start)
  {
    // Nope, start it now...
    pid_t	server_pid;		// Server process ID
    posix_spawnattr_t server_attrs;	// Server process attributes
    char * const server_argv[] =	// Server command-line
    {
      _papplMainloopPath,
      "server",
      "-o",
      "private-server=true",
      NULL
    };

    posix_spawnattr_init(&server_attrs);
    posix_spawnattr_setpgroup(&server_attrs, 0);

    if (posix_spawn(&server_pid, _papplMainloopPath, NULL, &server_attrs, server_argv, environ))
    {
      fprintf(stderr, "%s: Unable to start server: %s\n", base_name, strerror(errno));
      posix_spawnattr_destroy(&server_attrs);
      return (NULL);
    }

    posix_spawnattr_destroy(&server_attrs);

    // Wait for it to start...
    _papplMainloopGetServerPath(base_name, getuid(), sockname, sizeof(sockname));

    do
    {
      usleep(250000);
    }
    while (access(sockname, 0));

    http = httpConnect2(sockname, 0, NULL, AF_UNSPEC, HTTP_ENCRYPTION_IF_REQUESTED, 1, 30000, NULL);

    if (!http)
      fprintf(stderr, "%s: Unable to connect to server: %s\n", base_name, cupsLastErrorString());
  }
#endif // _WIN32

  return (http);
}


//
// '_papplMainloopConnectURI()' - Connect to an IPP printer directly.
//

http_t *				// O - HTTP connection or `NULL` on error
_papplMainloopConnectURI(
    const char *base_name,		// I - Base Name
    const char *printer_uri,		// I - Printer URI
    char       *resource,		// I - Resource path buffer
    size_t     rsize)			// I - Size of buffer
{
  char			scheme[32],	// Scheme (ipp or ipps)
			userpass[256],	// Username/password (unused)
			hostname[256];	// Hostname
  int			port;		// Port number
  http_encryption_t	encryption;	// Type of encryption to use
  http_t		*http;		// HTTP connection


  // First extract the components of the URI...
  if (httpSeparateURI(HTTP_URI_CODING_ALL, printer_uri, scheme, sizeof(scheme), userpass, sizeof(userpass), hostname, sizeof(hostname), &port, resource, (int)rsize) < HTTP_URI_STATUS_OK)
  {
    fprintf(stderr, "%s: Bad printer URI '%s'.\n", base_name, printer_uri);
    return (NULL);
  }

  if (strcmp(scheme, "ipp") && strcmp(scheme, "ipps"))
  {
    fprintf(stderr, "%s: Unsupported URI scheme '%s'.\n", base_name, scheme);
    return (NULL);
  }

  if (userpass[0])
    fprintf(stderr, "%s: Warning - user credentials are not supported in URIs.\n", base_name);

  if (!strcmp(scheme, "ipps") || port == 443)
    encryption = HTTP_ENCRYPTION_ALWAYS;
  else
    encryption = HTTP_ENCRYPTION_IF_REQUESTED;

  if ((http = httpConnect2(hostname, port, NULL, AF_UNSPEC, encryption, 1, 30000, NULL)) == NULL)
    fprintf(stderr, "%s: Unable to connect to printer at '%s:%d': %s\n", base_name, hostname, port, cupsLastErrorString());

  return (http);
}


//
// '_papplMainloopGetDefaultPrinter' - Get the default printer.
//

char *					// O - Default printer or `NULL` for none
_papplMainloopGetDefaultPrinter(
    http_t *http,			// I - HTTP connection
    char   *buffer,			// I - Buffer for printer name
    size_t bufsize)			// I - Size of buffer
{
  ipp_t		*request,		// IPP request
		*response;		// IPP response
  const char	*printer_name;		// Printer name


  // Ask the server for its default printer
  request = ippNewRequest(IPP_OP_CUPS_GET_DEFAULT);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, cupsUser());
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_KEYWORD, "requested-attributes", NULL, "printer-name");

  response = cupsDoRequest(http, request, "/ipp/system");

  if ((printer_name = ippGetString(ippFindAttribute(response, "printer-name", IPP_TAG_NAME), 0, NULL)) != NULL)
    strlcpy(buffer, printer_name, bufsize);
  else
    *buffer = '\0';

  ippDelete(response);

  return (*buffer ? buffer : NULL);
}


//
// '_papplMainloopGetServerPath()' - Get the UNIX domain socket for the server.
//

char *					// O - Socket filename
_papplMainloopGetServerPath(
    const char *base_name,		// I - Base name
    uid_t      uid,			// I - UID for server
    char       *buffer,			// I - Buffer for filename
    size_t     bufsize)			// I - Size of buffer
{
  const char	*snap_common;		// SNAP_COMMON environment variable


  if (uid)
  {
    // Per-user server...
    const char	*tmpdir = getenv("TMPDIR");
					// Temporary directory

#ifdef __APPLE__
    if (!tmpdir)
      tmpdir = "/private/tmp";
#else
    if (!tmpdir)
      tmpdir = "/tmp";
#endif // __APPLE__

    snprintf(buffer, bufsize, "%s/%s%d.sock", tmpdir, base_name, (int)uid);
  }
  else if ((snap_common = getenv("SNAP_COMMON")) != NULL)
  {
    // System server running as root inside a snap (https://snapcraft.io)...
    snprintf(buffer, bufsize, "%s/%s.sock", snap_common, base_name);
  }
  else
  {
#if _WIN32
    // System server running as local service
    strlcpy(buffer, "localhost", bufsize);
#else
    // System server running as root
    snprintf(buffer, bufsize, PAPPL_SOCKDIR "/%s.sock", base_name);
#endif // _WIN32
  }

  _PAPPL_DEBUG("Using domain socket '%s'.\n", buffer);

  return (buffer);
}


//
// 'get_length()' - Get a length in hundreths of millimeters.
//

static int				// O - Length value
get_length(const char *value)		// I - Length string
{
  double	n;			// Number
  char		*units;			// Pointer to units


  n = strtod(value, &units);

  if (units && !strcmp(units, "cm"))
    return ((int)(n * 1000.0));
  if (units && !strcmp(units, "in"))
    return ((int)(n * 2540.0));
  else if (units && !strcmp(units, "mm"))
    return ((int)(n * 100.0));
  else if (units && !strcmp(units, "m"))
    return ((int)(n * 100000.0));
  else
    return ((int)n);
}
