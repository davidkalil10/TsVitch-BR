

#include "view/video_card.hpp"
#include "view/svg_image.hpp"
#include "view/text_box.hpp"
#include "utils/number_helper.hpp"
#include "utils/image_helper.hpp"
#include "core/FavoriteManager.hpp"
#include <pystring.h>

using namespace brls::literals;

void BaseVideoCard::prepareForReuse() { this->picture->setImageFromRes("pictures/video-card-bg.png"); }

void BaseVideoCard::cacheForReuse() { ImageHelper::clear(this->picture); }

RecyclingGridItemLiveVideoCard::RecyclingGridItemLiveVideoCard() {
    this->inflateFromXMLRes("xml/views/video_card_live.xml");
}

RecyclingGridItemLiveVideoCard::~RecyclingGridItemLiveVideoCard() { ImageHelper::clear(this->picture); }

void RecyclingGridItemLiveVideoCard::setChannel(tsvitch::LiveM3u8 liveData) {
    this->liveData = liveData;
    this->labelGroup->setText(liveData.groupTitle);
    this->labelTitle->setIsWrapping(false);
    this->labelTitle->setText(liveData.title);
    ImageHelper::with(this->picture)->load(liveData.logo);

    bool isFavorite = FavoriteManager::get()->isFavorite(liveData.url);

    if (isFavorite) {
        this->svgFavoriteIcon->setImageFromSVGRes("svg/ico-favorite-activate.svg");
        this->svgFavoriteIcon->setVisibility(brls::Visibility::VISIBLE);
    } else
        this->svgFavoriteIcon->setVisibility(brls::Visibility::GONE);

    this->labelChno->setText(liveData.chno);
}

tsvitch::LiveM3u8 RecyclingGridItemLiveVideoCard::getChannel() { return this->liveData; }

void RecyclingGridItemLiveVideoCard::setFavoriteIcon(bool isFavorite) {
    if (isFavorite) {
        this->svgFavoriteIcon->setImageFromSVGRes("svg/ico-favorite-activate.svg");
        this->svgFavoriteIcon->setVisibility(brls::Visibility::VISIBLE);
    } else
        this->svgFavoriteIcon->setVisibility(brls::Visibility::GONE);
}

RecyclingGridItemLiveVideoCard* RecyclingGridItemLiveVideoCard::create() {
    return new RecyclingGridItemLiveVideoCard();
}

RecyclingGridItemVodCard::RecyclingGridItemVodCard() {
    this->inflateFromXMLRes("xml/views/video_card_poster.xml");
}

RecyclingGridItemVodCard::~RecyclingGridItemVodCard() { 
    if (this->picture) ImageHelper::clear(this->picture);
}

void RecyclingGridItemVodCard::setVod(tsvitch::XtreamVod vodData) {
    this->vodData = vodData;
    
    if (this->labelTitle) {
        this->labelTitle->setIsWrapping(true);
        this->labelTitle->setText(vodData.name);
    }
    
    if (this->picture) {
        if (!vodData.stream_icon.empty()) {
            ImageHelper::with(this->picture)->load(vodData.stream_icon);
        } else {
            this->picture->setImageFromRes("pictures/video-card-bg.png"); // Default/Placeholder
        }
    }
    
    // Show rating if available
    if (this->labelRating) {
        if (!vodData.rating.empty()) {
            this->labelRating->setText(vodData.rating);
            this->labelRating->setVisibility(brls::Visibility::VISIBLE);
        } else {
            this->labelRating->setVisibility(brls::Visibility::GONE);
        }
    }
}

tsvitch::XtreamVod RecyclingGridItemVodCard::getVod() { return this->vodData; }

RecyclingGridItemVodCard* RecyclingGridItemVodCard::create() {
    return new RecyclingGridItemVodCard();
}

RecyclingGridItemSeriesCard::RecyclingGridItemSeriesCard() {
    this->inflateFromXMLRes("xml/views/video_card_poster.xml");
}

RecyclingGridItemSeriesCard::~RecyclingGridItemSeriesCard() { 
    if (this->picture) ImageHelper::clear(this->picture);
}

void RecyclingGridItemSeriesCard::setSeries(tsvitch::XtreamSeries seriesData) {
    this->seriesData = seriesData;
    
    if (this->labelTitle) {
        this->labelTitle->setIsWrapping(true);
        this->labelTitle->setText(seriesData.name);
    }
    
    if (this->picture) {
        if (!seriesData.cover.empty()) {
            ImageHelper::with(this->picture)->load(seriesData.cover);
        } else {
            this->picture->setImageFromRes("pictures/video-card-bg.png");
        }
    }
    
    // Show rating if available
    if (this->labelRating) {
        if (!seriesData.rating.empty()) {
            this->labelRating->setText(seriesData.rating);
            this->labelRating->setVisibility(brls::Visibility::VISIBLE);
        } else {
            this->labelRating->setVisibility(brls::Visibility::GONE);
        }
    }
}

tsvitch::XtreamSeries RecyclingGridItemSeriesCard::getSeries() { return this->seriesData; }

RecyclingGridItemSeriesCard* RecyclingGridItemSeriesCard::create() {
    return new RecyclingGridItemSeriesCard();
}

