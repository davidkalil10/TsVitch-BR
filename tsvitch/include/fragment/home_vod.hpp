#ifndef HOME_VOD_HPP
#define HOME_VOD_HPP

#include "view/auto_tab_frame.hpp"
#include <borealis/core/box.hpp>
#include <borealis/core/bind.hpp>
#include "view/recycling_grid.hpp"
#include "utils/xtream_helper.hpp"
#include "view/custom_button.hpp"

class HomeVod : public AttachedView {
public:
    HomeVod();
    ~HomeVod() override;

    static brls::View* create();

    void onShow() override;
    
    // Callbacks
    void onVodList(const std::vector<tsvitch::XtreamVod>& result);
    void onError(const std::string& error);

private:
    void loadData();
    void selectGroupIndex(size_t index);
    
    BRLS_BIND(RecyclingGrid, recyclingGrid, "home/vod/recyclingGrid");
    BRLS_BIND(RecyclingGrid, upRecyclingGrid, "dynamic/up/recyclingGrid");
    BRLS_BIND(CustomButton, searchField, "home/search");

    std::vector<tsvitch::XtreamVod> fullVodList;
    std::vector<tsvitch::XtreamCategory> categories; // Not used yet, we group by category_id or name
    
    std::mutex groupCacheMutex;
    std::map<std::string, std::vector<tsvitch::XtreamVod>> groupCache;
    int selectedGroupIndex = 0;
};

#endif // HOME_VOD_HPP
