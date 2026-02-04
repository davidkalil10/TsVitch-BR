#include "fragment/home_vod.hpp"
#include "view/video_card.hpp"
#include <borealis/core/application.hpp>
#include <borealis/core/thread.hpp>
#include <unordered_map>
#include <algorithm>

using namespace brls::literals;

// Reusing DynamicGroupChannels directly as it's generic enough (defined in HomeLive, need to see if it's accessible or re-define)
// Since it was defined in home_live.cpp (cpp file), it's NOT accessible. I must redefine it or move it to a header.
// For speed, I will redefine a similar class here or make a common header later. Redefining for now.

class DynamicGroupChannelsVod : public RecyclingGridItem {
public:
    explicit DynamicGroupChannelsVod(const std::string& xml) {
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
        return new DynamicGroupChannelsVod(xml);
    }

private:
    BRLS_BIND(brls::Label, labelTitle, "title");
    NVGcolor selectedColor{};
    NVGcolor fontColor{};
};

class DataSourceUpListVod : public RecyclingGridDataSource {
public:
    using OnGroupSelected = std::function<void(const std::string&)>;
    explicit DataSourceUpListVod(std::vector<std::string> result, OnGroupSelected cb = nullptr)
        : list(std::move(result)), onGroupSelected(cb) {}

    RecyclingGridItem* cellForRow(RecyclingGrid* recycler, size_t index) override {
        DynamicGroupChannelsVod* item = (DynamicGroupChannelsVod*)recycler->dequeueReusableCell("Cell");
        item->setTitle(this->list[index]);
        item->setSelected(index == selectedIndex);
        return item;
    }

    size_t getItemCount() override { return list.size(); }

    void setSelectedIndex(RecyclingGrid* recycler, size_t index) {
        if (index >= list.size()) return;
        selectedIndex = index;
        auto* item = dynamic_cast<DynamicGroupChannelsVod*>(recycler->getGridItemByIndex(index));
        if (item) item->setSelected(true);
        if (onGroupSelected) onGroupSelected(list[index]);
    }

    void onItemSelected(RecyclingGrid* recycler, size_t index) override {
        std::vector<RecyclingGridItem*>& items = recycler->getGridItems();
        for (auto& i : items) {
            auto* cell = dynamic_cast<DynamicGroupChannelsVod*>(i);
            if (cell) cell->setSelected(false);
        }
        selectedIndex = index;
        auto* item = dynamic_cast<DynamicGroupChannelsVod*>(recycler->getGridItemByIndex(index));
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

class DataSourceVodList : public RecyclingGridDataSource {
public:
    explicit DataSourceVodList(const std::vector<tsvitch::XtreamVod>& result) : videoList(result) {}
    
    RecyclingGridItem* cellForRow(RecyclingGrid* recycler, size_t index) override {
        tsvitch::XtreamVod& r = this->videoList[index];
        RecyclingGridItemVodCard* item = (RecyclingGridItemVodCard*)recycler->dequeueReusableCell("Cell");
        item->setVod(r);
        return item;
    }

    size_t getItemCount() override { return videoList.size(); }

    void onItemSelected(RecyclingGrid* recycler, size_t index) override {
        // TODO: Handle click (play VOD)
        brls::Logger::info("Clicked VOD: {}", videoList[index].name);
        // Intent::openVod or similar
    }

    void clearData() override { this->videoList.clear(); }

private:
    std::vector<tsvitch::XtreamVod> videoList;
};

HomeVod::HomeVod() {
    this->inflateFromXMLRes("xml/fragment/home_vod.xml");
    
    recyclingGrid->registerCell("Cell", []() { return RecyclingGridItemVodCard::create(); });
    upRecyclingGrid->registerCell("Cell", []() { return DynamicGroupChannelsVod::create(); });
    
    recyclingGrid->showSkeleton();
    upRecyclingGrid->setVisibility(brls::Visibility::GONE);
}

HomeVod::~HomeVod() {}

brls::View* HomeVod::create() { return new HomeVod(); }

#include "utils/config_helper.hpp"

// ... existing includes ...

void HomeVod::onShow() {
    if (!tsvitch::XtreamAPI::instance().isConfigured()) {
        auto& config = ProgramConfig::instance();
        std::string url = config.getXtreamServerUrl();
        std::string user = config.getXtreamUsername();
        std::string pass = config.getXtreamPassword();
        if (!url.empty() && !user.empty() && !pass.empty()) {
            tsvitch::XtreamAPI::instance().setCredentials(url, user, pass);
        }
    }

    if (fullVodList.empty()) {
        loadData();
    }
}

void HomeVod::loadData() {
    recyclingGrid->showSkeleton();
    tsvitch::XtreamAPI::instance().getAllVodStreams([this](const std::vector<tsvitch::XtreamVod>& vods, bool success, const std::string& error) {
        brls::sync([this, vods, success, error]() {
            if (success) {
                this->onVodList(vods);
            } else {
                this->onError(error);
            }
        });
    });
}

void HomeVod::onVodList(const std::vector<tsvitch::XtreamVod>& result) {
    if (result.empty()) {
        recyclingGrid->setEmpty();
        return;
    }
    
    this->fullVodList = result;
    
    // Grouping
    std::unordered_map<std::string, std::vector<tsvitch::XtreamVod>> groups;
    for (const auto& item : result) {
        std::string catId = item.category_id; // Using Category ID as group for now, ideally Category Name
        // Need mapping CatID -> CatName if possible, but XtreamVod only has category_id.
        // We might need to fetch Categories first to map names.
        // For now, use category_id or if API returns category_name in struct (XtreamVod doesn't seem to have category_name, check struct)
        // Checked struct: XtreamVod has category_id.
        // So we should group by category_id.
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
        upRecyclingGrid->setDataSource(new DataSourceUpListVod(groupIds, [this](const std::string& group) {
            std::lock_guard<std::mutex> lock(groupCacheMutex);
            if (groupCache.count(group)) {
                recyclingGrid->setDataSource(new DataSourceVodList(groupCache[group]));
            }
        }));
        selectGroupIndex(0);
    } else {
         recyclingGrid->setDataSource(new DataSourceVodList(result));
         upRecyclingGrid->setVisibility(brls::Visibility::GONE);
    }
}

void HomeVod::selectGroupIndex(size_t index) {
    auto* datasource = dynamic_cast<DataSourceUpListVod*>(upRecyclingGrid->getDataSource());
    if (datasource) {
        datasource->setSelectedIndex(upRecyclingGrid, index);
    }
}

void HomeVod::onError(const std::string& error) {
    recyclingGrid->setError(error);
}
