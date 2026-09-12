#ifndef VERSION_H
#define VERSION_H

#ifndef APP_NAME
#define APP_NAME "NSSpeaker"
#endif
#ifndef APP_VERSION
#define APP_VERSION "0.1"
#endif
#ifndef APP_BUILD_DATE
#define APP_BUILD_DATE "unknown"
#endif

static const char app_name[] = APP_NAME;
static const char app_version[] = APP_VERSION;
static const char app_build_date[] = APP_BUILD_DATE;
static const char app_version_info[] =
    APP_NAME " version: " APP_VERSION ", built: " APP_BUILD_DATE ".";

#endif
