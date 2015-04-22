#ifndef __DEFAULTS_H_INCL__
    #define __DEFAULTS_H_INCL__

const int MINIMUM_WIDTH = 962;
const int MINIMUM_HEIGHT = 600;
const int DEFAULT_WIDTH = MINIMUM_WIDTH; // 1024;
const int DEFAULT_HEIGHT = MINIMUM_HEIGHT; // 768;
const int RESIZE_ZONE = 2;
const int DRAG_ZONE_RIGHT_MARGIN = 60;
const int DRAG_ZONE_HEIGHT = 30;

const int CONTROL_HIDE_INTERVAL_MOUSE_OVER_MS = 10 * 60 * 1000;
const int CONTROL_HIDE_INTERVAL_MOUSE_AWAY_MS = 5 * 1000;

const char APP_NAME[] = "Popcorn Time";
const char COMPANY_NAME_MAC[] = "time4popcorn.eu";

const char STARTUP_URL[] = "http://beta.time4popcorn.eu/?version=0.4.4a&os=mac";

const char SETTINGS_KEY_DOWNLOAD_DIR[] = "DownloadDir";
const char SETTINGS_KEY_CLEAN_ON_EXIT[] = "CleanOnExit";
const char SETTINGS_KEY_FONT_SIZE[] = "FontSize";
const char SETTINGS_KEY_VOLUME[] = "Volume";
const char SETTINGS_DEFAULT_FONT_SIZE = 0;
const char SETTINGS_KEY_CONFIG[] = "info";
const bool CLEAN_ON_EXIT_DEFAULT = true;

const char SUBTITLES_OFF[] = "<Subtitles off>";
const char SUBTITLES_NA[] = "<No subtitles>";

const int PROXY_CHECK_TIMEOUT_MS = 5 * 1000;

const int TORRENT_INFO_UPDATES_PER_SECOND = 3;
const int TORRENT_PIECES_TO_PLAY = 7;
const int PAUSE_IF_BUFFER_HAVE_LESS_THAN_MS = 10 * 1000;
const int DONT_PAUSE_IF_JUST_STARTED_TIME_MS = 30 * 1000;
const char TK_STARTING_DOWNLOAD[] = "Starting download";
const char TK_DOWNLOADING[] = "Downloading";
const char TK_NET_BUFFERING[] = "Network Buffering";

#endif // __DEFAULTS_H_INCL__

