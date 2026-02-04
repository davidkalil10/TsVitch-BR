#ifndef HOME_SERIES_HPP
#define HOME_SERIES_HPP

#include "view/auto_tab_frame.hpp"
#include <borealis/core/box.hpp>
#include <borealis/core/bind.hpp>
#include "view/recycling_grid.hpp"
#include "utils/xtream_helper.hpp"
#include "view/custom_button.hpp"

class HomeSeries : public AttachedView {
public:
    HomeSeries();
    ~HomeSeries() override;

    static brls::View* create();

    void onShow() override;
    
    // Callbacks
    void onSeriesList(const std::vector<tsvitch::XtreamSeries>& result);
    void onError(const std::string& error);

private:
    void loadData();
    void selectGroupIndex(size_t index);
    
    BRLS_BIND(RecyclingGrid, recyclingGrid, "home/series/recyclingGrid");
    BRLS_BIND(RecyclingGrid, upRecyclingGrid, "dynamic/up/recyclingGrid");
    BRLS_BIND(CustomButton, searchField, "home/search");

    std::vector<tsvitch::XtreamSeries> fullSeriesList;
    
    std::mutex groupCacheMutex;
    std::map<std::string, std::vector<tsvitch::XtreamSeries>> groupCache;
    int selectedGroupIndex = 0;
};

#endif // HOME_SERIES_HPP
