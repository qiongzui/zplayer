#pragma once

#define EXPORT __declspec(dllexport)

enum MsgType {
    MsgType_DurationMs,
    MsgType_CurrentTimestampMs
};

EXPORT void* zplayer_create(void* surface);
EXPORT void zplayer_open(void* zplayer, const char* url);
EXPORT void zplayer_close(void** zplayer);
EXPORT void zplayer_play(void* zplayer);
EXPORT void zplayer_pause(void* zplayer);
EXPORT void zplayer_seek(void* zplayer, int timestamp);
EXPORT void zplayer_render(void* zplayer, void* frame);
EXPORT void zplayer_query(void* zplayer, MsgType msgtype, int* ret);