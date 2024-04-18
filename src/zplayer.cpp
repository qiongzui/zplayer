#include "zplayer.h"
#include "Client.h"

using namespace ZPlayer;


void* zplayer_create(void* surface) {
	return reinterpret_cast<void*>(new Client());
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