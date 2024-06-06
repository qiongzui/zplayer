#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

extern "C" {
    #include "libxml/parser.h"
    #include "libxml/tree.h"
    #include "libavutil/imgutils.h"
    #include "libswscale/swscale.h"
}

namespace ZPlayer {
    struct Screen {
        xmlChar* id;
        float width = 0.f;
        float height = 0.f;

        float x = -1.f;
        float y = -1.f;
        ~Screen() {
            if (id) {
                xmlFree(id);
                id = nullptr;
            }
        }
    };

    struct View : Screen {
        xmlChar* orientation;
        xmlChar* left_of;
        xmlChar* right_of;
        xmlChar* top_of;
        xmlChar* bottom_of;
        xmlChar* align_horizontal;
        xmlChar* align_vertical;
        ~View() {
            if (orientation) {
                xmlFree(orientation);
                orientation = nullptr;
            }
            if (left_of) {
                xmlFree(left_of);
                left_of = nullptr;
            }
            if (right_of) {
                xmlFree(right_of);
                right_of = nullptr;
            }
            if (top_of) {
                xmlFree(top_of);
                top_of = nullptr;
            }
            if (bottom_of) {
                xmlFree(bottom_of);
                bottom_of = nullptr;
            }
            if (align_horizontal) {
                xmlFree(align_horizontal);
                align_horizontal = nullptr;
            }
            if (align_vertical) {
                xmlFree(align_vertical);
                align_vertical = nullptr;
            }
        }
    };
    
    struct ThumbInfo {
        xmlChar* id;
        float width = 0.f;
        float height = 0.f;
        int padding = 0;
        ~ThumbInfo() {
            if (id) {
                xmlFree(id);
                id = nullptr;
            }
        }
    };

    struct ThumbScreenView : View {
        int max_thumb_count = 0;
        ThumbInfo thumb_info;
    };
    
    struct RecorderView : View {
        Screen main_screen;
        ThumbScreenView thumb_screen_view;
    };

    struct Font_color {
        int red = 0;
        int green = 0;
        int blue = 0;
        float alpha = 1.f;
    };

    struct TextScreen : Screen {
        xmlChar* id;
        int font_size = 0;
        Font_color font_color;
        ~TextScreen() {
            if (id) {
                xmlFree(id);
                id = nullptr;
            }
        }
    };
    
    
    struct TextView : View {
        TextScreen text_screen;
    };
    
    struct Layout {
        xmlChar* orientation;
        RecorderView recorder_view;
        TextView text_view;

        ~Layout() {
            if (orientation) {
                xmlFree(orientation);
                orientation = nullptr;
            }
        }
    };

    struct ScreenSize {
        int width = 0;
        int height = 0;
        int x = 0;
        int y = 0;
    };

    struct TextScreenInfo {
        ScreenSize screen_size;
        int font_size = 0;
        Font_color font_color;
    };

    class XmlParse {
    public:
        XmlParse();
        ~XmlParse();

        int parseXml(const char* file);
        int release();

        int resetOutputInfo(uint8_t* outBuffer, int outWidth, int outHeight, int outStride, AVPixelFormat outFormat, int thumbCnt);
        int addMainScreen(AVFrame* frame, int frameWidth, int frameHeight);
        int addThumbScreen(AVFrame* frame, int frameWidth, int frameHeight, int thumbIndex);
        int addTextScreen(AVFrame* frame, int frameWidth, int frameHeight);
        const TextScreenInfo& getTextScreenInfo() const;

    private:
        int parseRoot();
        int parseRecorderView(RecorderView* recorderView, xmlNodePtr node);
        int parseThumbScreenView(ThumbScreenView* thumbScreenView, xmlNodePtr node);
        int parseThumbInfo(ThumbInfo* thumbInfo, xmlNodePtr node);
        int parseTextView(TextView* textView, xmlNodePtr node);
        int parseTextScreen(TextScreen* textScreen, xmlNodePtr node);
        int parseTextColor(Font_color* font_color, xmlNodePtr node);
        int parseView(View* view, xmlNodePtr node);
        int parseScreen(Screen* screen, xmlNodePtr node);

        void generateOutputLayout();
        int generateRecorderViewSize();
        int generateTextViewSize();
        int generateThumbViewSize();
        int generateMainScreenSize();
        int generateThumbScreensSize();

        int addYUVMainScreen(uint8_t* mainBuffer, int mainStride);
        int addYUVThumbScreen(uint8_t* thumbBuffer, int thumbStride, int thumbIndex);
        int addYUVTextScreen(uint8_t* textBuffer, int textStride);

        int addRBGAMainScreen(uint8_t* mainBuffer, int mainStride);
        int addRBGAThumbScreen(uint8_t* thumbBuffer, int thumbStride, int thumbIndex);
        int addRBGATextScreen(uint8_t* textBuffer, int textStride);

        void clearInfo();
        
    private:

        xmlDocPtr _doc;
        xmlNodePtr _root;

        Layout _layout;
        const xmlChar* RECORDER_VIEW_NAME = BAD_CAST("recorder_view");
        const xmlChar* TITLE_VIEW_NAME = BAD_CAST("text_view");
        const xmlChar* MAIN_SCREEN_NAME = BAD_CAST("main_screen");
        const xmlChar* THUMB_SCREEN_VIEW_NAME = BAD_CAST("thumb_screen_view");
        const xmlChar* THUMB_INFO_NAME = BAD_CAST("thumb_info");

        int _thumbScreenIndex = 0;

        int _outputWidth = 0;
        int _outputHeight = 0;
        int _outputStride = 0;
        AVPixelFormat _outputFormat = AV_PIX_FMT_NONE;
        uint8_t* _outputBuffer = nullptr;

        ScreenSize _recorderViewSize;
        ScreenSize _mainScreenSize;
        ScreenSize _textViewSize;
        ScreenSize _thumbViewSize;
        std::vector<ScreenSize> _thumbScreenSizes;
        TextScreenInfo _textScreenInfo;

        SwsContext* _swsMainContext = nullptr;
        SwsContext* _swsThumbContext = nullptr;
        AVFrame* _swsFrame_main = nullptr;
        AVFrame* _swsFrame_thumb = nullptr;

        std::unordered_map<std::string, ScreenSize*> _controls;
    };
}