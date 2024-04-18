#pragma once

enum MsgType {
    MsgType_DurationMs,
    MsgType_CurrentTimestampMs
};

void* zplayer_create(void* surface);
void zplayer_open(void* zplayer, const char* url);
void zplayer_close(void** zplayer);
void zplayer_play(void* zplayer);
void zplayer_pause(void* zplayer);
void zplayer_seek(void* zplayer, int timestamp);

void zplayer_query(void* zplayer, MsgType msgtype, int* ret);