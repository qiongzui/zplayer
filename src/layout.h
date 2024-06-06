#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ff_video_filter.h"
extern "C" {
    #include "libswscale/swscale.h"
}

namespace ZPlayer {
    struct MainScreen {
        float left_top_x;
        float left_top_y;
        int width;
        int height;
    };

    struct ThumbScreen {
        float left_top_x;
        float left_top_y;
        int width;
        int height;

        int maxThumbImageCount;
        int padding;
    };

    struct FontInfo {
        int font_size;
        int font_color;
        int alpha;
    };
    
    struct TextScreen {
        float left_top_x;
        float left_top_y;
        int width;
        int height;

        FontInfo font_info;
    };
    
    class Layout {
    public:
        void setupLayoutJson(std::string json_path, int outWidth, int outHeight, int thumb_cnt, std::string classRoomName);
        void fillOutputImage(std::shared_ptr<Image> outImage, std::shared_ptr<Image> mainImage,
                             std::shared_ptr<Image> floatingImage, float floatingX, float floatingY,
                             std::vector<std::shared_ptr<Image>> thumbImages);
        const std::shared_ptr<TextScreen>& getTextScreen() { return _textScreen; }
    private:
        void copyImage(std::shared_ptr<Image> dst, std::shared_ptr<Image> src, int x, int y);

        bool _lastIsShowThumbs = false;
        std::shared_ptr<MainScreen> _mainScreen;
        std::shared_ptr<ThumbScreen> _thumbScreen;
        std::shared_ptr<TextScreen> _textScreen;

        SwsContext* _swsMainContext = nullptr;
        SwsContext* _swsThumbContext = nullptr;

        std::shared_ptr<Image> _mainTmpImage = nullptr;
        std::shared_ptr<Image> _thumbTmpImage = nullptr;

        std::string _textName;
        std::unique_ptr<FF_VideoFilter> _textFilter = nullptr;
    };
}