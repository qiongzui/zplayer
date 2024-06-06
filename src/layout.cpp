#include "layout.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <Windows.h>

#include "nlohmann/json.hpp"

using namespace ZPlayer;
// std::wstring UTF8ToUnicode(const std::string& str)
// {
//     std::wstring ret;
//     try {
//         std::wstring_convert< std::codecvt_utf8<wchar_t> > wcv;
//         ret = wcv.from_bytes(str);
//     }
//     catch (const std::exception& e) {
//         std::cerr << e.what() << std::endl;
//     }
//     return ret;
// }

std::string UnicodeToUtf8(std::string strSrc)
{
  int nwLen = ::MultiByteToWideChar(CP_ACP, 0, strSrc.c_str(), -1, NULL, 0);
  wchar_t *pwBuf = new wchar_t[nwLen + 1];
  ZeroMemory(pwBuf, nwLen * 2 + 2);
  ::MultiByteToWideChar(CP_ACP, 0, strSrc.c_str(), strSrc.length(), pwBuf, nwLen);
  int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
  char * pBuf = new char[nLen + 1];
  ZeroMemory(pBuf, nLen + 1);
  ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
  std::string sResult(pBuf);
  delete[] pwBuf;
  delete[] pBuf;
  pwBuf = nullptr;
  pBuf = nullptr;
  return sResult;
}

void Layout::setupLayoutJson(std::string json_path, int outWidth, int outHeight, int thumb_cnt, std::string classRoomName) {
    if (json_path.empty()) {
        
    }
    _textName = classRoomName;
    std::ifstream ifs(json_path.c_str());
    nlohmann::json json = nlohmann::json::parse(ifs);

    _mainScreen = std::make_shared<MainScreen>();
    _thumbScreen = std::make_shared<ThumbScreen>();
    _textScreen = std::make_shared<TextScreen>();

    _mainScreen->left_top_x = json["mainScreen"]["left_top_x"];
    _mainScreen->left_top_y = json["mainScreen"]["left_top_y"];
    _mainScreen->width = (float)json["mainScreen"]["parcent_width"] * outWidth;
    _mainScreen->height = (float)json["mainScreen"]["parcent_height"] * outHeight;

    _thumbScreen->left_top_x = json["thumbScreen"]["left_top_x"];
    _thumbScreen->left_top_y = json["thumbScreen"]["left_top_y"];
    _thumbScreen->width = (float)json["thumbScreen"]["parcent_width"] * outWidth;
    _thumbScreen->height = (float)json["thumbScreen"]["parcent_height"] * outHeight;
    _thumbScreen->maxThumbImageCount = json["thumbScreen"]["maxThumbImageCount"];
    _thumbScreen->padding = json["thumbScreen"]["padding"];

    _textScreen->left_top_x = json["textScreen"]["left_top_x"];
    _textScreen->left_top_y = json["textScreen"]["left_top_y"];
    _textScreen->width = (float)json["textScreen"]["parcent_width"] * outWidth;
    _textScreen->height = (float)json["textScreen"]["parcent_height"] * outHeight;

    _textScreen->font_info.font_size = json["textScreen"]["fontInfo"]["fontSize"];
    std::string color = json["textScreen"]["fontInfo"]["fontColor"];
    std::stringstream colorStream;
    colorStream << std::hex << color;
    colorStream >> _textScreen->font_info.font_color;
    _textScreen->font_info.alpha = json["textScreen"]["fontInfo"]["alpha"];
}

void Layout::fillOutputImage(std::shared_ptr<Image> outImage,
                             std::shared_ptr<Image> mainImage,
                             std::shared_ptr<Image> floatingImage, float floatingX, float floatingY,
                             std::vector<std::shared_ptr<Image>> thumbImages) {
    bool isShowThumbs = !thumbImages.empty();
    if (isShowThumbs != _lastIsShowThumbs) {
        _mainTmpImage = nullptr;
        _thumbTmpImage = nullptr;
        _lastIsShowThumbs = isShowThumbs;
    }

    int mainWidth = isShowThumbs? _mainScreen->width : _mainScreen->width  + _thumbScreen->width;

    if (!_mainTmpImage) {
        _mainTmpImage = std::make_shared<Image>();
        if (mainImage->width / _mainScreen->width > mainImage->height / _mainScreen->height) {
            _mainTmpImage->height = mainWidth * mainImage->height / mainImage->width;
            _mainTmpImage->width = mainWidth;
        } else {
            _mainTmpImage->width = _mainScreen->height * mainImage->width / mainImage->height;
            _mainTmpImage->height = _mainScreen->height;
        }

        if (outImage->format == AV_PIX_FMT_YUV420P) {
            _mainTmpImage->stride[0] = _mainTmpImage->width;
            _mainTmpImage->data[0] = new uint8_t[_mainTmpImage->width * _mainTmpImage->height];
            _mainTmpImage->stride[1] = _mainTmpImage->width / 2;
            _mainTmpImage->data[1] = new uint8_t[_mainTmpImage->width * _mainTmpImage->height / 4];
            _mainTmpImage->stride[2] = _mainTmpImage->width / 2;
            _mainTmpImage->data[2] = new uint8_t[_mainTmpImage->width * _mainTmpImage->height / 4];
        } else if (outImage->format == AV_PIX_FMT_RGBA) {
            _mainTmpImage->stride[0] = _mainTmpImage->width * 4;
            _mainTmpImage->data[0] = new uint8_t[_mainTmpImage->width * _mainTmpImage->height * 4];
        }
    }

    if (!_thumbTmpImage && isShowThumbs) {
        _thumbTmpImage = std::make_shared<Image>();
        _thumbTmpImage->width = _thumbScreen->width;
        _thumbTmpImage->height = _thumbScreen->width * 9 / 16;
        if (outImage->format == AV_PIX_FMT_YUV420P) {
            _thumbTmpImage->stride[0] = _thumbTmpImage->width;
            _thumbTmpImage->data[0] = new uint8_t[_thumbTmpImage->width * _thumbTmpImage->height];
            _thumbTmpImage->stride[1] = _thumbTmpImage->width / 2;
            _thumbTmpImage->data[1] = new uint8_t[_thumbTmpImage->width * _thumbTmpImage->height / 4];
            _thumbTmpImage->stride[2] = _thumbTmpImage->width / 2;
            _thumbTmpImage->data[2] = new uint8_t[_thumbTmpImage->width * _thumbTmpImage->height / 4];
        } else if (outImage->format == AV_PIX_FMT_RGBA) {
            _thumbTmpImage->stride[0] = _thumbTmpImage->width * 4;
            _thumbTmpImage->data[0] = new uint8_t[_thumbTmpImage->width * _thumbTmpImage->height * 4];
        }
    }

    if (outImage->format == AV_PIX_FMT_YUV420P) {
        int y_white = 0.257*255 + 0.504*255 + 0.098*255+16;
        int u_white = -0.148*255 - 0.291*255 + 0.439*255+128;
        int v_white = 0.439*255 - 0.368*255 - 0.071*255+128;
        memset(outImage->data[0], y_white, outImage->height * outImage->stride[0]);
        memset(outImage->data[1], u_white, outImage->height * outImage->stride[1] / 2);
        memset(outImage->data[2], v_white, outImage->height * outImage->stride[2] / 2);
    } else {
        memset(outImage->data[0], 255, outImage->height * outImage->stride[0]);
    }
    for (int ch = 0; ch < 3; ++ch) {
        if (!outImage->data[ch]) continue;
        memset(outImage->data[ch], 255, outImage->height * outImage->stride[ch]);
    }
    if (!_textFilter) {
        _textFilter = std::make_unique<FF_VideoFilter>();
        if (_textFilter) {
            std::string text = _textName;
            int textNameX = _textScreen->left_top_x + 5;
            int textNameY = _textScreen->left_top_y + (_textScreen->height - _textScreen->font_info.font_size) / 2;
            int textDateX = textNameX + _textScreen->font_info.font_size * text.length() + 10;
            int textDateY = textNameY;
            std::stringstream desc;
            
            desc << "drawtext=fontfile='C\\:/Windows/Fonts/simhei.ttf':"
                << "fontcolor=0x" << std::hex << std::setw(6) << std::setfill('0') << _textScreen->font_info.font_color << std::dec
                << "@" << _textScreen->font_info.alpha << ":fontsize=" << _textScreen->font_info.font_size
                << ":x=" << textNameX << ":y=" << textNameY
                << ":text='" << UnicodeToUtf8(text) << "',"
                << "drawtext=fontcolor=0x" << std::hex << std::setw(6) << std::setfill('0') << _textScreen->font_info.font_color << std::dec
                << "@" << _textScreen->font_info.alpha << ":fontsize=" << _textScreen->font_info.font_size
                << ":x=" << textDateX << ":y=" << textDateY << ":textfile='C\\:/Users/51917/Desktop/test/text.txt':reload=1";
                //<< ":text='%{localtime}'";
            if (_textFilter->init(outImage->width, outImage->height, static_cast<AVPixelFormat>(outImage->format), desc.str().c_str()) != 0) {
                _textFilter = nullptr;
                return;
            }
        }
    }

    int mainDstX = _mainScreen->left_top_x * outImage->width + (mainWidth - _mainTmpImage->width) / 2;
    int mainDstY = _mainScreen->left_top_y * outImage->height + (_mainScreen->height - _mainTmpImage->height) / 2;

    if (mainImage->width != _mainTmpImage->width || mainImage->height != _mainTmpImage->height) {
        if (_swsMainContext == nullptr) {
            _swsMainContext = sws_getContext(mainImage->width, mainImage->height, static_cast<AVPixelFormat>(mainImage->format),
                                            _mainTmpImage->width, _mainTmpImage->height, static_cast<AVPixelFormat>(outImage->format),
                                            SWS_BICUBIC, nullptr, nullptr, nullptr);
        }

        int height = sws_scale(_swsMainContext, mainImage->data, mainImage->stride,
                  0, mainImage->height, _mainTmpImage->data, _mainTmpImage->stride);

        copyImage(outImage, _mainTmpImage, mainDstX, mainDstY);
    } else {
        copyImage(outImage, mainImage, mainDstX, mainDstY);
    }

    int first_y = isShowThumbs? _thumbScreen->left_top_y * outImage->height + (_thumbScreen->height - _thumbTmpImage->height * thumbImages.size()) / 2 : 0;
    for (int i = 0; i < thumbImages.size(); ++i) {
        int thumbDstX = _thumbScreen->left_top_x * outImage->width;
        int thumbDstY = first_y + i * _thumbTmpImage->height;

        if (thumbImages[i]->width != _thumbTmpImage->width || thumbImages[i]->height != _thumbTmpImage->height) {
            if (_swsThumbContext == nullptr) {
                _swsThumbContext = sws_getContext(thumbImages[i]->width, thumbImages[i]->height, static_cast<AVPixelFormat>(thumbImages[i]->format),
                                                  _thumbTmpImage->width, _thumbTmpImage->height, static_cast<AVPixelFormat>(outImage->format),
                                                  SWS_BICUBIC, nullptr, nullptr, nullptr);
            }

            sws_scale(_swsThumbContext, thumbImages[i]->data, thumbImages[i]->stride,
                      0, thumbImages[i]->height, _thumbTmpImage->data, _thumbTmpImage->stride);
            copyImage(outImage, _thumbTmpImage, thumbDstX, thumbDstY);
        } else {
            copyImage(outImage, thumbImages[i], thumbDstX, thumbDstY);
        }
    }

    if (floatingImage) {
        int floatingDstX = floatingX * _mainScreen->width + _mainScreen->left_top_x * outImage->width;
        int floatingDstY = floatingY * _mainScreen->height + _mainScreen->left_top_y * outImage->height;
        
        if (floatingImage->width != _thumbTmpImage->width || floatingImage->height != _thumbTmpImage->height) {
            if (_swsThumbContext == nullptr) {
                _swsThumbContext = sws_getContext(floatingImage->width, floatingImage->height, static_cast<AVPixelFormat>(floatingImage->format),
                                                  _thumbTmpImage->width, _thumbTmpImage->height, static_cast<AVPixelFormat>(outImage->format),
                                                  SWS_BICUBIC, nullptr, nullptr, nullptr);
            }

            sws_scale(_swsThumbContext, floatingImage->data, floatingImage->stride,
                      0, floatingImage->height, _thumbTmpImage->data, _thumbTmpImage->stride);
            copyImage(outImage, _thumbTmpImage, floatingDstX, floatingDstY);
        } else {
            copyImage(outImage, floatingImage, floatingDstX, floatingDstY);
        }
    }

    _textFilter->filter(outImage, outImage);
}

void Layout::copyImage(std::shared_ptr<Image> dst, std::shared_ptr<Image> src, int x, int y) {
    for (int ch = 0; ch < 3; ++ch) {
        if (!src->data[ch]) continue;
        for (int i = 0; i < src->height; ++i) {
            if (y + i >= dst->height) break;
            memcpy(dst->data[ch] + (y + i) * dst->stride[ch] + x * 4, src->data[ch] + i * src->stride[ch], src->width * 4);
        }
    }

}