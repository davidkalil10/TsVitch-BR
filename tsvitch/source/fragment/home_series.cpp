#include "fragment/home_series.hpp"
#include "view/video_card.hpp"
#include <borealis/core/application.hpp>
#include <borealis/core/thread.hpp>
#include <unordered_map>
#include <algorithm>

using namespace brls::literals;

class DynamicGroupChannelsSeries : public RecyclingGridItem {
public:
    explicit DynamicGroupChannelsSeries(const std::string& xml) {
        this->inflateFromXMLRes(xml);
        auto theme    = brls::Application::getTheme();
        selectedColor = theme.getColor("color/tsvitch");
        fontColor     = theme.getColor("brls/text");
    }

    void setTitle(const std::string& title) { this->labelTitle->setText(title); }
    void setSelected(bool selected) { this->labelTitle->setTextColor(selected ? selectedColor : fontColor); }

    void prepareForReuse() override {
        this->labelTitle->setText("");
        this->labelTitle->setTextColor(fontColor);
    }
    void cacheForReuse() override {}

    static RecyclingGridItem* create(const std::string& xml = "xml/views/group_channel_dynamic.xml") {
        return new DynamicGroupChannelsSeries(xml);
    }

private:
    BRLS_BIND(brls::Label, labelTitle, "title");
    NVGcolor selectedColor{};
    NVGcolor fontColor{};
};

class DataSourceUpListSeries : public RecyclingGridDataSource {
public:
    using OnGroupSelected = std::function<void(const std::string&)>;
    explicit DataSourceUpListSeries(std::vector<std::string> result, OnGroupSelected cb = nullptr)
        : list(std::move(result)), onGroupSelected(cb) {}

    RecyclingGridItem* cellForRow(RecyclingGrid* recycler, size_t index) override {
        DynamicGroupChannelsSeries* item = (DynamicGroupChannelsSeries*)recycler->dequeueReusableCell("Cell");
        item->setTitle(this->list[index]);
        item->setSelected(index == selectedIndex);
        return item;
    }

    size_t getItemCount() override { return list.size(); }

    void setSelectedIndex(RecyclingGrid* recycler, size_t index) {
        if (index >= list.size()) return;
        selectedIndex = index;
        auto* item = dynamic_cast<DynamicGroupChannelsSeries*>(recycler->getGridItemByIndex(index));
        if (item) item->setSelected(true);
        if (onGroupSelected) onGroupSelected(list[index]);
    }

    void onItemSelected(RecyclingGrid* recycler, size_t index) override {
        std::vector<RecyclingGridItem*>& items = recycler->getGridItems();
        for (auto& i : items) {
            auto* cell = dynamic_cast<DynamicGroupChannelsSeries*>(i);
            if (cell) cell->setSelected(false);
        }
        selectedIndex = index;
        auto* item = dynamic_cast<DynamicGroupChannelsSeries*>(recycler->getGridItemByIndex(index));
        if (item) item->setSelected(true);
        if (onGroupSelected) onGroupSelected(list[index]);
    }
    
    std::string getGroupNameByIndex(size_t index) {
         if (index < list.size()) return list[index];
         return "";
    }

    void clearData() override { this->list.clear(); }

private:
    std::vector<std::string> list;
    size_t selectedIndex = -1;
    OnGroupSelected onGroupSelected;
};

class DataSourceSeriesList : public RecyclingGridDataSource {
public:
    explicit DataSourceSeriesList(const std::vector<tsvitch::XtreamSeries>& result) : seriesList(result) {}
    
    RecyclingGridItem* cellForRow(RecyclingGrid* recycler, size_t index) override {
        tsvitch::XtreamSeries& r = this->seriesList[index];
        RecyclingGridItemSeriesCard* item = (RecyclingGridItemSeriesCard*)recycler->dequeueReusableCell("Cell");
        item->setSeries(r);
        return item;
    }

    size_t getItemCount() override { return seriesList.size(); }

    void onItemSelected(RecyclingGrid* recycler, size_t index) override {
        // TODO: Handle click (play Series/Episodes)
        brls::Logger::info("Clicked Series: {}", seriesList[index].name);
    }

    void clearData() override { this->seriesList.clear(); }

private:
    std::vector<tsvitch::XtreamSeries> seriesList;
};

HomeSeries::HomeSeries() {
    this->inflateFromXMLRes("xml/fragment/home_series.xml");
    
    recyclingGrid->registerCell("Cell", []() { return RecyclingGridItemSeriesCard::create(); });
    upRecyclingGrid->registerCell("Cell", []() { return DynamicGroupChannelsSeries::create(); });
    
    recyclingGrid->showSkeleton();
    upRecyclingGrid->setVisibility(brls::Visibility::GONE);
}

HomeSeries::~HomeSeries() {}

brls::View* HomeSeries::create() { return new HomeSeries(); }

#include "utils/config_helper.hpp"

// ... existing includes ...

void HomeSeries::onShow() {
    if (!tsvitch::XtreamAPI::instance().isConfigured()) {
        auto& config = ProgramConfig::instance();
        std::string url = config.getXtreamServerUrl();
        std::string user = config.getXtreamUsername();
        std::string pass = config.getXtreamPassword();
        if (!url.empty() && !user.empty() && !pass.empty()) {
            tsvitch::XtreamAPI::instance().setCredentials(url, user, pass);
        }
    }

    if (fullSeriesList.empty()) {
        loadData();
    }
}

void HomeSeries::loadData() {
    recyclingGrid->showSkeleton();
    tsvitch::XtreamAPI::instance().getAllSeries([this](const std::vector<tsvitch::XtreamSeries>& seriesList, bool success, const std::string& error) {
        brls::sync([this, seriesList, success, error]() {
            if (success) {
                this->onSeriesList(seriesList);
            } else {
                this->onError(error);
            }
        });
    });
}

void HomeSeries::onSeriesList(const std::vector<tsvitch::XtreamSeries>& result) {
    if (result.empty()) {
        recyclingGrid->setEmpty();
        return;
    }
    
    this->fullSeriesList = result;
    
    // Grouping
    std::unordered_map<std::string, std::vector<tsvitch::XtreamSeries>> groups;
    for (const auto& item : result) {
        std::string catId = item.category_id;
        groups[catId].push_back(item);
    }
    
    std::vector<std::string> groupIds;
    for (const auto& pair : groups) groupIds.push_back(pair.first);
    std::sort(groupIds.begin(), groupIds.end());
    
    // Populate cache
    {
        std::lock_guard<std::mutex> lock(groupCacheMutex);
        groupCache.clear();
        for(auto& pair : groups) {
            groupCache[pair.first] = pair.second;
        }
    }
    
    if (groupIds.size() > 1) {
        upRecyclingGrid->setVisibility(brls::Visibility::VISIBLE);
        upRecyclingGrid->setDataSource(new DataSourceUpListSeries(groupIds, [this](const std::string& group) {
            std::lock_guard<std::mutex> lock(groupCacheMutex);
            if (groupCache.count(group)) {
                recyclingGrid->setDataSource(new DataSourceSeriesList(groupCache[group]));
            }
        }));
        selectGroupIndex(0);
    } else {
         recyclingGrid->setDataSource(new DataSourceSeriesList(result));
         upRecyclingGrid->setVisibility(brls::Visibility::GONE);
    }
}

void HomeSeries::selectGroupIndex(size_t index) {
    auto* datasource = dynamic_cast<DataSourceUpListSeries*>(upRecyclingGrid->getDataSource());
    if (datasource) {
        datasource->setSelectedIndex(upRecyclingGrid, index);
    }
}

void HomeSeries::onError(const std::string& error) {
    recyclingGrid->setError(error);
}
