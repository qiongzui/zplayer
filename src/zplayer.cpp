#include "zplayer.h"
#include "engine.h"

using namespace ZPlayer;


void* zplayer_create(void* surface) {
	auto client = new Client();
	client->setSurface(surface);
	return client;
}

void zplayer_open(void* zplayer, const char* url) {
	auto client = reinterpret_cast<Client*>(zplayer);
	if (!client) {
		return;
	}
	client->setDataSource(url);
	client->prepare();
}

void zplayer_close(void** zplayer) {
	auto client = reinterpret_cast<Client*>(*zplayer);
	if (!client) {
		return;
	}

	client->stop();
	client->release();
	delete *zplayer;
	*zplayer = nullptr;
}

void zplayer_play(void* zplayer) {
	auto client = reinterpret_cast<Client*>(zplayer);
	if (!client) {
		return;
	}
	client->play();
}

void zplayer_pause(void* zplayer) {
	auto client = reinterpret_cast<Client*>(zplayer);
	if (!client) {
		return;
	}
	client->pause();
}

void zplayer_seek(void* zplayer, int timestamp) {
	auto client = reinterpret_cast<Client*>(zplayer);
	if (!client) {
		return;
	}
	client->seek(timestamp);
}

void zplayer_query(void* zplayer, MsgType msgtype, int* ret) {
	auto client = reinterpret_cast<Client*>(zplayer);
	if (!client) {
		return;
	}

	switch (msgtype) {
	case MsgType_DurationMs:
		*ret = client->getDurationMs();
		break;
	case MsgType_CurrentTimestampMs:
		*ret = client->getCurrentTimestampMs();
		break;
	default:
		break;
	}
}

void zplayer_render(void* zplayer, void* frame) {
	auto client = reinterpret_cast<Client*>(zplayer);
	if (!client) {
		return;
	}
	client->render();
}