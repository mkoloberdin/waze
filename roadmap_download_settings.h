#ifndef ROADMAP_DOWNLOAD_SETTINGS_H_
#define ROADMAP_DOWNLOAD_SETTINGS_H_
#include "ssd/ssd_widget.h"
void roadmap_download_settings_show(void);
void roadmap_download_settings_init(void);
BOOL roadmap_download_settings_isEnabled(RoadMapConfigDescriptor descriptor);
BOOL roadmap_download_settings_isDownloadWazers(void);
BOOL roadmap_download_settings_isDownloadReports(void);
BOOL roadmap_download_settings_isDownloadTraffic(void);
void roadmap_download_settings_setDownloadTraffic(BOOL is_enabled);
#endif /*ROADMAP_DOWNLOAD_SETTINGS_H_*/
