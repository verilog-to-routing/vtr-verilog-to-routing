#ifndef KEYS_H
#define KEYS_H

static const char* KEY_JOB_ID = "JOB_ID";
static const char* KEY_CMD = "CMD";
static const char* KEY_OPTIONS = "OPTIONS";
static const char* KEY_DATA = "DATA";
static const char* KEY_STATUS = "STATUS";
static const char* END_TELEGRAM_SEQUENCE = "###___END_FRAME___###";

enum CMD {
    CMD_GET_PATH_LIST_ID=0,
    CMD_DRAW_PATH_ID
};

#endif
