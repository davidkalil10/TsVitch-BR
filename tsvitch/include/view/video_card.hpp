

#pragma once

#include "view/recycling_grid.hpp"
#include "api/tsvitch/result/home_live_result.h" 
#include "utils/xtream_helper.hpp" 
#include "utils/xtream_helper.hpp" 

class SVGImage;
class TextBox;

class BaseVideoCard : public RecyclingGridItem {
public:
    void prepareForReuse() override;

    void cacheForReuse() override;

protected:
    BRLS_BIND(brls::Image, picture, "video/card/picture");
};

class RecyclingGridItemLiveVideoCard : public BaseVideoCard {
public:
    RecyclingGridItemLiveVideoCard();

    ~RecyclingGridItemLiveVideoCard() override;

    void setChannel(tsvitch::LiveM3u8 liveData);

   tsvitch::LiveM3u8 getChannel();

                                             void setFavoriteIcon(bool isFavorite);

    static RecyclingGridItemLiveVideoCard* create();

private:
tsvitch::LiveM3u8 liveData;
    BRLS_BIND(TextBox, labelTitle, "video/card/label/title");
    BRLS_BIND(brls::Label, labelGroup, "video/card/label/group");
    BRLS_BIND(brls::Label, labelChno, "video/card/label/chno");
    BRLS_BIND(brls::Box, boxPic, "video/card/pic_box");
    BRLS_BIND(brls::Box, boxHint, "video/card/hint");
    BRLS_BIND(SVGImage, svgUp, "video/svg/up");
    BRLS_BIND(SVGImage, svgFavoriteIcon, "video/card/ico/favorite");
};

class RecyclingGridItemVodCard : public BaseVideoCard {
public:
    RecyclingGridItemVodCard();
    ~RecyclingGridItemVodCard() override;
    void setVod(tsvitch::XtreamVod vodData);
    tsvitch::XtreamVod getVod();
    static RecyclingGridItemVodCard* create();
private:
    tsvitch::XtreamVod vodData;
    BRLS_BIND(TextBox, labelTitle, "video/card/label/title");
    BRLS_BIND(brls::Label, labelRating, "video/card/label/rating"); 
    BRLS_BIND(brls::Box, boxPic, "video/card/pic_box");
};

class RecyclingGridItemSeriesCard : public BaseVideoCard {
public:
    RecyclingGridItemSeriesCard();
    ~RecyclingGridItemSeriesCard() override;
    void setSeries(tsvitch::XtreamSeries seriesData);
    tsvitch::XtreamSeries getSeries();
    static RecyclingGridItemSeriesCard* create();
private:
    tsvitch::XtreamSeries seriesData;
    BRLS_BIND(TextBox, labelTitle, "video/card/label/title");
    BRLS_BIND(brls::Label, labelRating, "video/card/label/rating");
    BRLS_BIND(brls::Box, boxPic, "video/card/pic_box");
};
