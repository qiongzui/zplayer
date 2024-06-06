#include "xml_parse.h"
#include <string>
#include <algorithm>

using namespace ZPlayer;

#define DEBUG_LAYOUT 1

XmlParse::XmlParse() {
    _doc = nullptr;
    _root = nullptr;
}

XmlParse::~XmlParse() {
    release();
}

int XmlParse::release() {
    clearInfo();
    if (_doc) {
        xmlFreeDoc(_doc);
        _doc = nullptr;
    }

    _layout = {0};
    _root = nullptr;
    _doc = nullptr;

    return 0;
}

int XmlParse::parseXml(const char* file) {
    _doc = xmlReadFile(file, "UTF-8", 0);
    if (!_doc) {
        return -1;
    }
    
    _root = xmlDocGetRootElement(_doc);
    if (!_root) {
        return -1;
    }

    _layout = {0};
    return parseRoot();
}

int XmlParse::parseRoot() {
    if (!_root) {
        return -1;
    }

    auto next = _root->xmlChildrenNode;
    int ret = -1;
    while (next) {
        if (xmlStrcmp(next->name, RECORDER_VIEW_NAME) == 0) {
            ret = parseRecorderView(&_layout.recorder_view, next);
            if (ret != 0) {
                return -1;
            }
        } else if (xmlStrcmp(next->name, TITLE_VIEW_NAME) == 0) {
            parseTextView(&_layout.text_view, next);
        } else if (xmlStrcmp(next->name, BAD_CAST("orientation")) == 0) {
            _layout.orientation = xmlNodeListGetString(_doc, next, 1);
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseRecorderView(RecorderView* recorderView, xmlNodePtr node) {
    if (!recorderView || !node) {
        return -1;
    }
    parseView(recorderView, node);

    recorderView->main_screen = {0};
    recorderView->thumb_screen_view = {};
    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, MAIN_SCREEN_NAME) == 0) {
            parseScreen(&recorderView->main_screen, next);
        } else if (xmlStrcmp(next->name, THUMB_SCREEN_VIEW_NAME) == 0) {
            parseThumbScreenView(&recorderView->thumb_screen_view, next);
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseTextView(TextView* textView, xmlNodePtr node) {
    if (!textView || !node) {
        return -1;
    }
    parseView(textView, node);
    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, BAD_CAST("text_screen")) == 0) {
            parseTextScreen(&textView->text_screen, next);
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseTextScreen(TextScreen* textScreen, xmlNodePtr node) {
    if (!textScreen || !node) {
        return -1;
    }

    parseScreen(textScreen, node);
    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, BAD_CAST("id")) == 0) {
            textScreen->id = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("font_size")) == 0) {
            textScreen->font_size = std::stoi((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("font_color")) == 0) {
            parseTextColor(&(textScreen->font_color), next);
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseTextColor(Font_color* font_color, xmlNodePtr node) {
    if (!font_color || !node) {
        return -1;
    }
    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, BAD_CAST("red")) == 0) {
            font_color->red = std::stoi((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("green")) == 0) {
            font_color->green = std::stoi((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("blue")) == 0) {
            font_color->blue = std::stoi((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("alpha")) == 0) {
            font_color->alpha = std::stof((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseScreen(Screen* screen, xmlNodePtr node) {
    if (!screen || !node) {
        return -1;
    }

    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, BAD_CAST("id")) == 0) {
            screen->id = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("width")) == 0) {
            screen->width = std::stof((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("height")) == 0) {
            screen->height = std::stof((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("x")) == 0) {
            screen->x = std::stof((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("y")) == 0) {
            screen->y = std::stof((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseView(View* view, xmlNodePtr node) {
    if (!view || !node) {
        return -1;
    }

    parseScreen(view, node);

    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, BAD_CAST("orientation")) == 0) {
            view->orientation = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("right_of")) == 0) {
            view->right_of = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("left_of")) == 0) {
            view->left_of = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("top_of")) == 0) {
            view->top_of = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("bottom_of")) == 0) {
            view->bottom_of = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("align_horizontal")) == 0) {
            view->align_horizontal = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("align_vertical")) == 0) {
            view->align_vertical = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseThumbInfo(ThumbInfo* thumbInfo, xmlNodePtr node) {
    if (!thumbInfo || !node) {
        return -1;
    }

    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, BAD_CAST("id")) == 0) {
            thumbInfo->id = xmlNodeListGetString(_doc, next->xmlChildrenNode, 1);
        } else if (xmlStrcmp(next->name, BAD_CAST("width")) == 0) {
            thumbInfo->width = std::stof((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("height")) == 0) {
            thumbInfo->height = std::stof((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("padding")) == 0) {
            thumbInfo->padding = std::stoi((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        }
        next = next->next;
    }
    return 0;
}

int XmlParse::parseThumbScreenView(ThumbScreenView* thumbScreenView, xmlNodePtr node) {
    if (!thumbScreenView || !node) {
        return -1;
    }

    parseView(thumbScreenView, node);

    auto next = node->xmlChildrenNode;
    while (next) {
        if (xmlStrcmp(next->name, BAD_CAST("max_thumb_count")) == 0) {
            thumbScreenView->max_thumb_count = std::stoi((char*) xmlNodeListGetString(_doc, next->xmlChildrenNode, 1));
        } else if (xmlStrcmp(next->name, BAD_CAST("thumb_info")) == 0) {
            parseThumbInfo(&thumbScreenView->thumb_info, next);
        }
        next = next->next;
    }
    return 0;
}

void XmlParse::clearInfo() {
    _thumbScreenIndex = 0;

    _outputWidth = 0;
    _outputHeight = 0;
    _outputStride = 0;
    _outputBuffer = nullptr;

    _recorderViewSize = {0};
    _mainScreenSize = {0};
    _textViewSize = {0};
    _thumbViewSize = {0};
    _thumbScreenSizes.clear();

    if (_swsMainContext) {
        sws_freeContext(_swsMainContext);
        _swsMainContext = nullptr;
    }

    if (_swsThumbContext) {
        sws_freeContext(_swsThumbContext);
        _swsThumbContext = nullptr;
    }

    if (_swsFrame_main) {
        av_frame_free(&_swsFrame_main);
        _swsFrame_main = nullptr;
    }

    if (_swsFrame_thumb) {
        av_frame_free(&_swsFrame_thumb);
        _swsFrame_thumb = nullptr;
    }
}

int XmlParse::resetOutputInfo(uint8_t* outBuffer, int outWidth, int outHeight, int outStride, AVPixelFormat outFormat, int thumbCnt) {
    if (thumbCnt > _layout.recorder_view.thumb_screen_view.max_thumb_count) {
        return -1;
    }

    clearInfo();

    _outputBuffer = outBuffer;
    _outputWidth = outWidth;
    _outputHeight = outHeight;
    _outputStride = outStride;
    _outputFormat = outFormat;

    _swsFrame_main = av_frame_alloc();
    _swsFrame_thumb = av_frame_alloc();

    _thumbScreenSizes.resize(thumbCnt);

    generateOutputLayout();
}

int XmlParse::generateRecorderViewSize() {
    _recorderViewSize = {0};
    auto& recorderView = _layout.recorder_view;

    _recorderViewSize.width = _outputWidth * recorderView.width;
    _recorderViewSize.height = _outputHeight * recorderView.height;
    if (recorderView.x > -1 && recorderView.y > -1) {
        _recorderViewSize.x = recorderView.x * _outputWidth;
        _recorderViewSize.y = recorderView.y * _outputHeight;

        if (_recorderViewSize.x + _recorderViewSize.width > _outputWidth) {
            _recorderViewSize.width = _outputWidth - _recorderViewSize.x;
        }
        if (_recorderViewSize.y + _recorderViewSize.height > _outputHeight) {
            _recorderViewSize.height = _outputHeight - _recorderViewSize.y;
        }
    } else {
        if (xmlStrcmp(recorderView.align_horizontal, BAD_CAST("center")) == 0) {
            _recorderViewSize.x = (_outputWidth - _recorderViewSize.width) / 2;
        } else if (xmlStrcmp(recorderView.align_horizontal, BAD_CAST("left")) == 0) {
            _recorderViewSize.x = 0;
        } else if (xmlStrcmp(recorderView.align_horizontal, BAD_CAST("right")) == 0) {
            _recorderViewSize.x = _outputWidth - _recorderViewSize.width;
        }
        if (xmlStrcmp(recorderView.align_vertical, BAD_CAST("center")) == 0) {
            _recorderViewSize.y = (_outputHeight - _recorderViewSize.height) / 2;
        } else if (xmlStrcmp(recorderView.align_vertical, BAD_CAST("top")) == 0) {
            _recorderViewSize.y = 0;
        } else if (xmlStrcmp(recorderView.align_vertical, BAD_CAST("bottom")) == 0) {
            _recorderViewSize.y = _outputHeight - _recorderViewSize.height;
        }
    }
#ifdef DEBUG_LAYOUT
    for (int i = _recorderViewSize.y; i < _recorderViewSize.y + _recorderViewSize.height; i++) {
        memset(_outputBuffer + i * _outputStride + _recorderViewSize.x * 4, 200, _recorderViewSize.width * 4);
    }
#endif

    // main screen
    generateMainScreenSize();

    // thumb view
    generateThumbViewSize();

    return 0;
}

int XmlParse::generateTextViewSize() {
    _textViewSize = {0};
    auto& textView = _layout.text_view;
    _textViewSize.width = _outputWidth * textView.width;
    _textViewSize.height = _outputHeight * textView.height;
    if (textView.x > -1 && textView.y > -1) {
        _textViewSize.x = textView.x * _outputWidth;
        _textViewSize.y = textView.y * _outputHeight;
        if (_textViewSize.x + _textViewSize.width > _outputWidth) {
            _textViewSize.width = _outputWidth - _textViewSize.x;
        }
        if (_textViewSize.y + _textViewSize.height > _outputHeight) {
            _textViewSize.height = _outputHeight - _textViewSize.y;
        }
    } else if (textView.x > -1) {
        _textViewSize.x = textView.x * _recorderViewSize.width + _recorderViewSize.x;
        if (textView.top_of) {
            std::string key = reinterpret_cast<char*>(textView.top_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto control = iter->second;
                _textViewSize.y = std::max(control->y - _textViewSize.height, 0);
            }
        } else if (textView.bottom_of) {
            std::string key = reinterpret_cast<char*>(textView.bottom_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto control = iter->second;
                _textViewSize.y = control->y + control->height;
            }
        }
    } else if (textView.y > -1) {
        _textViewSize.y = textView.y * _recorderViewSize.height + _recorderViewSize.y;
        if (textView.right_of) {
            std::string key = reinterpret_cast<char*>(textView.right_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto control = iter->second;
                _textViewSize.x = control->x + control->width;
            }
        } else if (textView.left_of) {
            std::string key = reinterpret_cast<char*>(textView.left_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto controlSize = iter->second;
                _textViewSize.x = std::max(controlSize->x - _textViewSize.width, 0);
            }
        }
    }

    _textScreenInfo = { _textViewSize.width, _textViewSize.height,_textViewSize.x, _textViewSize.y,
                         textView.text_screen.font_size, textView.text_screen.font_color};

#ifdef DEBUG_LAYOUT
    for (int i = _textViewSize.y; i < _textViewSize.y + _textViewSize.height; i++) {
        memset(_outputBuffer + i * _outputStride + _textViewSize.x * 4, 255, _textViewSize.width * 4);
    }
#endif
    if (textView.id) {
        _controls.emplace(reinterpret_cast<char*>(textView.id), &_textViewSize);
    }
    
    return 0;
}

int XmlParse::generateThumbViewSize() {
    _thumbViewSize = {0};
    auto& thumbView = _layout.recorder_view.thumb_screen_view;
    _thumbViewSize.width = _recorderViewSize.width * thumbView.width;
    _thumbViewSize.height = _recorderViewSize.height * thumbView.height;
    if (thumbView.x > -1 && thumbView.y > -1) {
        _thumbViewSize.x = thumbView.x * _recorderViewSize.width + _recorderViewSize.x;
        _thumbViewSize.y = thumbView.y * _recorderViewSize.height + _recorderViewSize.y;
        if (_thumbViewSize.x + _thumbViewSize.width > _recorderViewSize.width) {
            _thumbViewSize.width = _recorderViewSize.width - _thumbViewSize.x;
        }
        if (_thumbViewSize.y + _thumbViewSize.height > _recorderViewSize.height) {
            _thumbViewSize.height = _recorderViewSize.height - _thumbViewSize.y;
        }
    } else if (thumbView.x > -1) {
        _thumbViewSize.x = thumbView.x * _recorderViewSize.width + _recorderViewSize.x;
        if (thumbView.top_of) {
            std::string key = reinterpret_cast<char*>(thumbView.top_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto control = iter->second;
                _thumbViewSize.y = std::max(control->y - _thumbViewSize.height, 0);
            }
        } else if (thumbView.bottom_of) {
            std::string key = reinterpret_cast<char*>(thumbView.bottom_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto control = iter->second;
                _thumbViewSize.y = control->y + control->height;
            }
        }
    } else if (thumbView.y > -1) {
        _thumbViewSize.y = thumbView.y * _recorderViewSize.height + _recorderViewSize.y;
        if (thumbView.right_of) {
            std::string key = reinterpret_cast<char*>(thumbView.right_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto control = iter->second;
                _thumbViewSize.x = control->x + control->width;
            }
        } else if (thumbView.left_of) {
            std::string key = reinterpret_cast<char*>(thumbView.left_of);
            auto iter = _controls.find(key.c_str());
            if (iter != _controls.end()) {
                auto controlSize = iter->second;
                _thumbViewSize.x = std::max(controlSize->x - _thumbViewSize.width, 0);
            }
        }
    }
#ifdef DEBUG_LAYOUT
    for (int i = _thumbViewSize.y; i < _thumbViewSize.y + _thumbViewSize.height; i++) {
        memset(_outputBuffer + i * _outputStride + _thumbViewSize.x * 4, 100, _thumbViewSize.width * 4);
    }
#endif

    if (thumbView.id) {
        _controls.emplace(reinterpret_cast<char*>(thumbView.id), &_thumbViewSize);
    }
    
    // thumb screens
    generateThumbScreensSize();
    return 0;
}

int XmlParse::generateMainScreenSize() {
    _mainScreenSize = {0};
    auto& mainScreen = _layout.recorder_view.main_screen;
    _mainScreenSize.width = _recorderViewSize.width * mainScreen.width;
    _mainScreenSize.height = _recorderViewSize.height * mainScreen.height;

    if (mainScreen.x > -1 && mainScreen.y > -1) {
        _mainScreenSize.x = mainScreen.x * _recorderViewSize.width + _recorderViewSize.x;
        _mainScreenSize.y = mainScreen.y * _recorderViewSize.height + _recorderViewSize.y;
    } else if (mainScreen.x > -1) {
        _mainScreenSize.x = mainScreen.x * _recorderViewSize.width + _recorderViewSize.x;
        _mainScreenSize.y = 0;
    } else if (mainScreen.y > -1) {
        _mainScreenSize.x = 0;
        _mainScreenSize.y = mainScreen.y * _recorderViewSize.height + _recorderViewSize.y;
    }
    if (_mainScreenSize.x + _mainScreenSize.width > _recorderViewSize.width) {
        _mainScreenSize.width = _recorderViewSize.width - _mainScreenSize.x;
    }
    if (_mainScreenSize.y + _mainScreenSize.height > _recorderViewSize.height) {
        _mainScreenSize.height = _recorderViewSize.height - _mainScreenSize.y;
    }
#ifdef DEBUG_LAYOUT
    for (int i = _mainScreenSize.y; i < _mainScreenSize.y + _mainScreenSize.height; i++) {
        memset(_outputBuffer + i * _outputStride + _mainScreenSize.x * 4, 150, _mainScreenSize.width * 4);
    }
#endif
    if (mainScreen.id) {
        _controls.emplace(reinterpret_cast<char*>(mainScreen.id), &_mainScreenSize);
    }
    
    av_image_alloc(_swsFrame_main->data, _swsFrame_main->linesize, _mainScreenSize.width, _mainScreenSize.height, _outputFormat, 1);
    return 0;
}

int XmlParse::generateThumbScreensSize() {
    auto thumbCnt = _thumbScreenSizes.size();
    auto& thumbView = _layout.recorder_view.thumb_screen_view;
    auto& thumbInfo = thumbView.thumb_info;

    int thumbScreenWidth = _thumbViewSize.width * thumbInfo.width;
    int thumbScreenHeight = _thumbViewSize.height * thumbInfo.height;
    int start_x = _thumbViewSize.x;
    int start_y = _thumbViewSize.y;

    if (_thumbViewSize.width < thumbScreenWidth) {
        thumbScreenWidth = _thumbViewSize.width;
    }
    if (_thumbViewSize.height < thumbScreenHeight) {
        thumbScreenHeight = _thumbViewSize.height;
    }
    
    if (xmlStrcmp(thumbView.orientation, BAD_CAST("horizontal")) == 0) {
        if (_thumbViewSize.width < thumbScreenWidth * thumbCnt) {
            thumbScreenWidth = _thumbViewSize.width / thumbCnt;
        }
        if (xmlStrcmp(thumbView.align_horizontal, BAD_CAST("center")) == 0) {
            start_x = (_thumbViewSize.width - thumbScreenWidth * thumbCnt) / 2 + _thumbViewSize.x;
        } else if (xmlStrcmp(thumbView.align_horizontal, BAD_CAST("left")) == 0) {
            start_x = _thumbViewSize.x;
        } else if (xmlStrcmp(thumbView.align_horizontal, BAD_CAST("right")) == 0) {
            start_x = _thumbViewSize.x + _thumbViewSize.width - thumbScreenWidth * thumbCnt;
        }

        if (xmlStrcmp(thumbView.align_vertical, BAD_CAST("center")) == 0) {
            start_y = (_thumbViewSize.height - thumbScreenHeight) / 2 + _thumbViewSize.y;
        } else if (xmlStrcmp(thumbView.align_vertical, BAD_CAST("top")) == 0) {
            start_y = _thumbViewSize.y;
        } else if (xmlStrcmp(thumbView.align_vertical, BAD_CAST("bottom")) == 0) {
            start_y = _thumbViewSize.y + _thumbViewSize.height - thumbScreenHeight;
        }

        for (int i = 0; i < thumbCnt; i++) {
            _thumbScreenSizes[i].width = thumbScreenWidth - thumbInfo.padding * 2;
            _thumbScreenSizes[i].height = thumbScreenHeight - thumbInfo.padding * 2;
            _thumbScreenSizes[i].x = start_x + i * thumbScreenWidth + thumbInfo.padding;
            _thumbScreenSizes[i].y = start_y + thumbInfo.padding;
        }
    } else if (xmlStrcmp(thumbView.orientation, BAD_CAST("vertical")) == 0) {
        if (_thumbViewSize.height < thumbScreenHeight * thumbCnt) {
            thumbScreenHeight = _thumbViewSize.height / thumbCnt;
        }
        if (xmlStrcmp(thumbView.align_horizontal, BAD_CAST("center")) == 0) {
            start_x = (_thumbViewSize.width - thumbScreenWidth) / 2 + _thumbViewSize.x;
        } else if (xmlStrcmp(thumbView.align_horizontal, BAD_CAST("left")) == 0) {
            start_x = _thumbViewSize.x;
        } else if (xmlStrcmp(thumbView.align_horizontal, BAD_CAST("right")) == 0) {
            start_x = _thumbViewSize.x + _thumbViewSize.width - thumbScreenWidth;
        }

        if (xmlStrcmp(thumbView.align_vertical, BAD_CAST("center")) == 0) {
            start_y = (_thumbViewSize.height - thumbScreenHeight * thumbCnt) / 2 + _thumbViewSize.y;
        } else if (xmlStrcmp(thumbView.align_vertical, BAD_CAST("top")) == 0) {
            start_y = _thumbViewSize.y;
        } else if (xmlStrcmp(thumbView.align_vertical, BAD_CAST("bottom")) == 0) {
            start_y = _thumbViewSize.y + _thumbViewSize.height - thumbScreenHeight * thumbCnt;
        }

        for (int i = 0; i < thumbCnt; i++) {
            _thumbScreenSizes[i].width = thumbScreenWidth - thumbInfo.padding * 2;
            _thumbScreenSizes[i].height = thumbScreenHeight - thumbInfo.padding * 2;
            _thumbScreenSizes[i].x = start_x + thumbInfo.padding;
            _thumbScreenSizes[i].y = start_y + i * thumbScreenHeight + thumbInfo.padding;
        }
    }

    av_image_alloc(_swsFrame_thumb->data, _swsFrame_thumb->linesize, thumbScreenWidth, thumbScreenHeight, _outputFormat, 1);
    return 0;
}

void XmlParse::generateOutputLayout() {
    // recorder view
    generateRecorderViewSize();

    // text view
    generateTextViewSize();
}

int XmlParse::addYUVMainScreen(uint8_t* mainBuffer, int mainStride) {
    // copy y
    auto dst_addr_y = _outputBuffer + _mainScreenSize.y * _outputStride + _mainScreenSize.x;
    for (int i = _mainScreenSize.y; i < _mainScreenSize.y + _mainScreenSize.height; i++) {
        memcpy(dst_addr_y + i * _outputStride, mainBuffer + (i - _mainScreenSize.y) * mainStride, _mainScreenSize.width);
    }

    // copy uv
    auto main_addr_u = mainBuffer + _mainScreenSize.width * _mainScreenSize.height;
    auto main_addr_v = mainBuffer + _mainScreenSize.width * _mainScreenSize.height * 5 / 4;
    auto dst_addr_u = _outputBuffer + _outputStride * _outputHeight + _mainScreenSize.y * _outputStride / 4 + _mainScreenSize.x / 2;
    auto dst_addr_v = _outputBuffer + _outputStride * _outputHeight * 5 / 4 + _mainScreenSize.y * _outputStride / 4 + _mainScreenSize.x / 2;
    for (int i = _mainScreenSize.y / 4; i < (_mainScreenSize.y + _mainScreenSize.height) / 4; i++) {
        int main_line = i - _mainScreenSize.y / 4;
        memcpy(dst_addr_u + i * _outputStride, main_addr_u + main_line * mainStride, _mainScreenSize.width / 2);
        memcpy(dst_addr_u + i * _outputStride + _outputWidth / 2, main_addr_u + main_line * mainStride + _mainScreenSize.width / 2, _mainScreenSize.width / 2);
    
        memcpy(dst_addr_v + i * _outputStride, main_addr_v + main_line * mainStride, _mainScreenSize.width / 2);
        memcpy(dst_addr_v + i * _outputStride + _outputWidth / 2, main_addr_v + main_line * mainStride + _mainScreenSize.width / 2, _mainScreenSize.width / 2);
    }
    return 0;
}

int XmlParse::addYUVThumbScreen(uint8_t* thumbBuffer, int thumbStride, int thumbIndex) {
    if (thumbIndex >= _thumbScreenSizes.size()) {
        return -1;
    }

    auto thumbSize = _thumbScreenSizes[thumbIndex];

    int thumbX = thumbSize.x;
    int thumbY = thumbSize.y;
    // copy y
    for (int i = thumbY; i < thumbY + thumbSize.height; i++) {
        memcpy(_outputBuffer + i * _outputStride + thumbX, thumbBuffer + (i - thumbY) * thumbStride, thumbSize.width);
    }

    // copy uv
    auto thumb_addr_u = thumbBuffer + thumbSize.width * thumbSize.height;
    auto thumb_addr_v = thumbBuffer + thumbSize.width * thumbSize.height * 5 / 4;
    auto dst_addr_u = _outputBuffer + _outputStride * _outputHeight + thumbX / 2;
    auto dst_addr_v = _outputBuffer + _outputStride * _outputHeight * 5 / 4 + thumbX / 2;
    for (int i = thumbY / 4; i < (thumbY + thumbSize.height) / 4; i++) {
        int thumb_line = i - thumbY / 4;
        memcpy(dst_addr_u + i * _outputStride, thumb_addr_u + thumb_line * thumbStride, thumbSize.width / 2);
        memcpy(dst_addr_u + i * _outputStride + _outputWidth / 2, thumb_addr_u + thumb_line * thumbStride + thumbSize.width / 2, thumbSize.width / 2);
    
        memcpy(dst_addr_v + i * _outputStride, thumb_addr_v + thumb_line * thumbStride, thumbSize.width / 2);
        memcpy(dst_addr_v + i * _outputStride + _outputWidth / 2, thumb_addr_v + thumb_line * thumbStride + thumbSize.width / 2, thumbSize.width / 2);
    }
}

int XmlParse::addYUVTextScreen(uint8_t* textBuffer, int textStride) {
    for (int i = _textViewSize.y; i < _textViewSize.y + _textViewSize.height; i++) {
        memcpy(_outputBuffer + i * _outputStride + _textViewSize.x, textBuffer + (i - _textViewSize.y) * textStride, _textViewSize.width);
    }
    return 0;
}

int XmlParse::addRBGAMainScreen(uint8_t* mainBuffer, int mainStride) {
    for (int i = _mainScreenSize.y; i < _mainScreenSize.y + _mainScreenSize.height; i++) {
        memcpy(_outputBuffer + i * _outputStride + _mainScreenSize.x * 4, mainBuffer + (i - _mainScreenSize.y) * mainStride, _mainScreenSize.width * 4);
    }

    return 0;
}

int XmlParse::addRBGAThumbScreen(uint8_t* thumbBuffer, int thumbStride, int thumbIndex) {
    if (thumbIndex >= _thumbScreenSizes.size()) {
        return -1;
    }

    auto thumbSize = _thumbScreenSizes[thumbIndex];

    int thumbX = thumbSize.x;
    int thumbY = thumbSize.y;
    // copy
    for (int i = thumbY; i < thumbY + thumbSize.height; i++) {
        memcpy(_outputBuffer + i * _outputStride + thumbX * 4, thumbBuffer + (i - thumbY) * thumbStride, thumbSize.width * 4);
    }

    return 0;
}

int XmlParse::addRBGATextScreen(uint8_t* textBuffer, int textStride) {
    for (int i = _textViewSize.y; i < _textViewSize.y + _textViewSize.height; i++) {
        memcpy(_outputBuffer + i * _outputStride + _textViewSize.x * 4, textBuffer + (i - _textViewSize.y) * textStride, _textViewSize.width * 4);
    }
    return 0;
}

int XmlParse::addThumbScreen(AVFrame* frame, int frameWidth, int frameHeight, int thumbIndex) {
    if (frameWidth != _thumbScreenSizes[thumbIndex].width || frameHeight != _thumbScreenSizes[thumbIndex].height) {
        if (!_swsThumbContext) {
            _swsThumbContext = sws_getContext(frameWidth, frameHeight, static_cast<AVPixelFormat>(frame->format), _thumbScreenSizes[thumbIndex].width, _thumbScreenSizes[thumbIndex].height, _outputFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
        }
        if (_swsThumbContext) {
            sws_scale(_swsThumbContext, frame->data, frame->linesize, 0, frameHeight, _swsFrame_thumb->data, _swsFrame_thumb->linesize);
        }
    }
    if (_outputFormat == AV_PIX_FMT_YUV420P) {
        return addYUVThumbScreen((uint8_t*)_swsFrame_thumb->data[0], _swsFrame_thumb->linesize[0], thumbIndex);
    } else if (_outputFormat == AV_PIX_FMT_RGBA) {
        return addRBGAThumbScreen((uint8_t*)_swsFrame_thumb->data[0], _swsFrame_thumb->linesize[0], thumbIndex);
    } else {
        return -1;
    }
    return 0;
}

int XmlParse::addMainScreen(AVFrame* frame, int frameWidth, int frameHeight) {
    if (frameWidth != _mainScreenSize.width || frameHeight != _mainScreenSize.height) {
        if (!_swsMainContext) {
            _swsMainContext = sws_getContext(frameWidth, frameHeight, static_cast<AVPixelFormat>(frame->format), _mainScreenSize.width, _mainScreenSize.height, _outputFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
        }
        if (_swsMainContext) {
            sws_scale(_swsMainContext, frame->data, frame->linesize, 0, frameHeight, _swsFrame_main->data, _swsFrame_main->linesize);
        }
    }

    if (_outputFormat == AV_PIX_FMT_YUV420P) {
        return addYUVMainScreen((uint8_t*)_swsFrame_main->data[0], _swsFrame_main->linesize[0]);
    } else if (_outputFormat == AV_PIX_FMT_RGBA) {
        return addRBGAMainScreen((uint8_t*)_swsFrame_main->data[0], _swsFrame_main->linesize[0]);
    } else {
        return -1;
    }
    return 0;
}

const TextScreenInfo& XmlParse::getTextScreenInfo() const{
    return _textScreenInfo;
}

int XmlParse::addTextScreen(AVFrame* frame, int frameWidth, int frameHeight) {
    if (_outputFormat == AV_PIX_FMT_YUV420P) {
        return addYUVTextScreen((uint8_t*)frame->data[0], frame->linesize[0]);
    } else if (_outputFormat == AV_PIX_FMT_RGBA) {
        return addRBGATextScreen((uint8_t*)frame->data[0], frame->linesize[0]);
    } else {
        return -1;
    }
    return 0;
}