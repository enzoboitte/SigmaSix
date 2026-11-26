#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include <SigmaUi/Ui.hpp>
#include <algorithm>
#include <tuple>

namespace ui {

namespace {

std::unordered_map<std::string, Style> g_styles;
int g_textSize = 10;

const char* kAnchors[] = {"TopLeft", "Top", "TopRight", "Left", "Center", "Right", "BottomLeft", "Bottom", "BottomRight"};

Vector2 vec2(const yaml::Node& n, Vector2 def)
{
    auto v = n.asFloats();
    return v.size() >= 2 ? Vector2{v[0], v[1]} : def;
}

int color(const yaml::Node& n)
{
    std::string s = n.asString();
    if (s.empty()) return 0;
    if (s[0] == '#') s.erase(0, 1);
    if (s.size() == 6) s += "ff";
    return static_cast<int>(std::stoul(s, nullptr, 16));
}

Style parseStyle(const yaml::Node& n)
{
    Style s;
    s.textSize = static_cast<int>(n["TextSize"].asInt());
    const std::string align = n["Align"].asString();
    s.align = align == "Left" ? TEXT_ALIGN_LEFT : align == "Center" ? TEXT_ALIGN_CENTER : align == "Right" ? TEXT_ALIGN_RIGHT : -1;
    s.textColor = color(n["TextColor"]);
    s.baseColor = color(n["BaseColor"]);
    s.borderColor = color(n["BorderColor"]);
    s.backgroundColor = color(n["BackgroundColor"]);
    s.file = n["File"].asString();
    return s;
}

void readStyles(const yaml::Node& styles)
{
    for (std::size_t i = 0; i < styles.size() && styles.isMap(); ++i)
        g_styles[styles.keys()[i]] = parseStyle(styles.items()[i]);
}

int control(const std::string& t)
{
    if (t == "Label") return LABEL;
    if (t == "Button" || t == "LabelButton") return BUTTON;
    if (t == "Toggle" || t == "ToggleGroup") return TOGGLE;
    if (t == "CheckBox") return CHECKBOX;
    if (t == "ComboBox") return COMBOBOX;
    if (t == "Dropdown") return DROPDOWNBOX;
    if (t == "TextBox") return TEXTBOX;
    if (t == "ValueBox" || t == "Spinner") return VALUEBOX;
    if (t == "Slider" || t == "SliderBar") return SLIDER;
    if (t == "ProgressBar") return PROGRESSBAR;
    if (t == "StatusBar") return STATUSBAR;
    return DEFAULT;
}

// Applique un style le temps d'un dessin puis restaure l'état raygui.
class StyleScope {
public:
    StyleScope(const Element& e, float scale)
    {
        auto it = g_styles.find(e.style);
        const Style* s = it == g_styles.end() ? nullptr : &it->second;
        const int c = control(e.type);
        if (s && s->textSize) g_textSize = s->textSize;
        set(DEFAULT, TEXT_SIZE, static_cast<int>(g_textSize * scale + 0.5f));
        if (!s) return;
        if (s->align >= 0) set(c, TEXT_ALIGNMENT, s->align);
        if (s->textColor) set(c, TEXT_COLOR_NORMAL, s->textColor);
        if (s->baseColor) set(c, BASE_COLOR_NORMAL, s->baseColor);
        if (s->borderColor) set(c, BORDER_COLOR_NORMAL, s->borderColor);
        if (s->backgroundColor) set(DEFAULT, BACKGROUND_COLOR, s->backgroundColor);
    }
    ~StyleScope()
    {
        g_textSize = m_textSize;
        for (auto it = m_saved.rbegin(); it != m_saved.rend(); ++it) GuiSetStyle(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
    }

private:
    void set(int c, int p, int v)
    {
        m_saved.emplace_back(c, p, GuiGetStyle(c, p));
        GuiSetStyle(c, p, v);
    }
    int m_textSize = g_textSize;
    std::vector<std::tuple<int, int, int>> m_saved;
};

std::string join(const std::vector<std::string>& items)
{
    std::string out;
    for (const auto& s : items) out += (out.empty() ? "" : ";") + s;
    return out;
}

}

bool loadStyles(const std::string& path, std::string* error)
{
    yaml::Node doc;
    if (!yaml::loadFile(path, doc, error)) return false;
    readStyles(doc.has("Styles") ? doc["Styles"] : doc);
    return true;
}

Screen::~Screen()
{
    for (auto& [_, t] : m_textures) UnloadTexture(t);
}

bool Screen::load(const std::string& path, std::string* error)
{
    yaml::Node doc;
    if (!yaml::loadFile(path, doc, error)) return false;
    const auto slash = path.find_last_of("/\\");
    m_dir = slash == std::string::npos ? "" : path.substr(0, slash + 1);
    root = Element{};
    root.type = "Container";

    for (std::size_t i = 0; i < doc.size() && doc.isMap(); ++i) {
        const std::string& key = doc.keys()[i];
        const yaml::Node& node = doc.items()[i];
        if (key == "Styles") readStyles(node);
        else if (key == "Screen") {
            m_reference = vec2(node["Size"], {0, 0});
            const std::string mode = node["Scale"].asString();
            m_scaleMode = mode == "Fit" ? 1 : mode == "Expand" ? 2 : 0;
        } else {
            parseElement(node, root.children.emplace_back(), key);
        }
    }
    return true;
}

void Screen::parseElement(const yaml::Node& n, Element& e, const std::string& type)
{
    e.type = n["Type"].asString(type);
    e.id = n["Id"].asString();
    e.text = n["Text"].asString(n["Title"].asString());
    e.action = n["Action"].asString();
    e.style = n["Style"].asString();
    e.tooltip = n["Tooltip"].asString();
    e.items = n["Items"].asStrings();
    e.pos = vec2(n["Position"], {0, 0});
    e.size = vec2(n["Size"], {0, 0});
    const std::string anchor = n["Anchor"].asString("TopLeft");
    e.anchor = static_cast<int>(std::find(std::begin(kAnchors), std::end(kAnchors), anchor) - std::begin(kAnchors)) % 9;
    e.min = n["Min"].asFloat(0);
    e.max = n["Max"].asFloat(100);
    e.value = n["Value"].asFloat(n["Checked"].asBool() ? 1.f : 0.f);
    e.visible = n["Visible"].asBool(true);
    e.enabled = n["Enabled"].asBool(true);
    e.closable = n["Closable"].asBool(true);
    e.showValue = n["ShowValue"].asBool();
    const int tint = color(n["Tint"]);
    e.tint = tint ? GetColor(static_cast<unsigned>(tint)) : WHITE;
    if (e.type == "TextBox") {
        e.buffer.assign(static_cast<std::size_t>(n["MaxLength"].asInt(64)) + 1, '\0');
        std::copy_n(e.text.begin(), std::min(e.text.size(), e.buffer.size() - 1), e.buffer.begin());
    }
    e.image = n["Image"].asString();
    if (!e.image.empty() && !m_textures.count(e.image)) m_textures[e.image] = LoadTexture((m_dir + e.image).c_str());

    auto it = g_styles.find(e.style);
    if (it != g_styles.end() && !it->second.file.empty()) GuiLoadStyle((m_dir + it->second.file).c_str());

    for (const yaml::Node& child : n["Children"].items()) parseElement(child, e.children.emplace_back(), "");
}

Screen& Screen::on(const std::string& action, Handler handler)
{
    m_handlers[action] = std::move(handler);
    return *this;
}

Element* Screen::find(const std::string& id)
{
    std::function<Element*(Element&)> search = [&](Element& e) -> Element* {
        if (e.id == id) return &e;
        for (auto& c : e.children)
            if (Element* r = search(c)) return r;
        return nullptr;
    };
    return id.empty() ? nullptr : search(root);
}

void Screen::setText(const std::string& id, const std::string& t)
{
    Element* e = find(id);
    if (!e) return;
    e->text = t;
    if (!e->buffer.empty()) {
        std::fill(e->buffer.begin(), e->buffer.end(), '\0');
        std::copy_n(t.begin(), std::min(t.size(), e->buffer.size() - 1), e->buffer.begin());
    }
}

void Screen::fire(Element& e)
{
    if (e.action.empty()) return;
    m_fired.insert(e.action);
    if (auto it = m_handlers.find(e.action); it != m_handlers.end()) it->second(e);
}

void Screen::draw()
{
    m_fired.clear();
    if (!visible) return;

    const float sw = static_cast<float>(GetScreenWidth()), sh = static_cast<float>(GetScreenHeight());
    float scale = 1;
    Rectangle area{0, 0, sw, sh};
    Vector2 offset{0, 0};
    if (m_scaleMode && m_reference.x > 0 && m_reference.y > 0) {
        scale = std::min(sw / m_reference.x, sh / m_reference.y);
        if (m_scaleMode == 1) {
            area = {0, 0, m_reference.x, m_reference.y};
            offset = {(sw - m_reference.x * scale) / 2, (sh - m_reference.y * scale) / 2};
        } else {
            area = {0, 0, sw / scale, sh / scale};
        }
    }
    m_offset = offset;

    const bool dropdownOpen = !m_late.empty() && std::any_of(m_late.begin(), m_late.end(), [](Element* e) { return e->edit; });
    m_late.clear();
    g_textSize = GuiGetStyle(DEFAULT, TEXT_SIZE);
    GuiEnableTooltip();
    if (dropdownOpen) GuiLock();
    layout(root, area, scale);
    drawElement(root, scale);
    if (dropdownOpen) GuiUnlock();
    for (Element* e : m_late) {
        StyleScope style(*e, scale);
        int active = static_cast<int>(e->value);
        if (GuiDropdownBox(e->bounds, join(e->items).c_str(), &active, e->edit)) {
            e->edit = !e->edit;
            if (!e->edit && active != static_cast<int>(e->value)) {
                e->value = static_cast<float>(active);
                fire(*e);
            }
        }
    }
    GuiSetTooltip(nullptr);
}

void Screen::layout(Element& e, Rectangle parent, float scale)
{
    Vector2 size = e.size;
    if (size.x <= 0 || size.y <= 0) {
        const int textSize = std::max(GuiGetStyle(DEFAULT, TEXT_SIZE), g_styles.count(e.style) ? g_styles[e.style].textSize : 0);
        if (e.type == "Container" || e.type == "Window" || e.type == "Panel") size = {parent.width, parent.height};
        else if (e.type == "Label") size = {static_cast<float>(MeasureText(e.text.c_str(), textSize)) + 8, textSize + 8.f};
        else if (e.type == "CheckBox") size = {20, 20};
        else if (e.type == "Image" && m_textures.count(e.image)) size = {float(m_textures[e.image].width), float(m_textures[e.image].height)};
        else size = {120, 30};
    }
    const float ax = (e.anchor % 3) / 2.f, ay = (e.anchor / 3) / 2.f;
    const Rectangle r{parent.x + parent.width * ax - size.x * ax + e.pos.x, parent.y + parent.height * ay - size.y * ay + e.pos.y, size.x, size.y};
    e.bounds = {m_offset.x + r.x * scale, m_offset.y + r.y * scale, r.width * scale, r.height * scale};
    for (auto& c : e.children) layout(c, r, scale);
}

void Screen::drawElement(Element& e, float scale)
{
    if (!e.visible) return;
    const int prevState = GuiGetState();
    if (!e.enabled) GuiSetState(STATE_DISABLED);
    GuiSetTooltip(e.tooltip.empty() ? nullptr : e.tooltip.c_str());

    {
        StyleScope style(e, scale);
        const Rectangle& b = e.bounds;
        const char* text = e.text.empty() ? nullptr : e.text.c_str();
        const std::string& t = e.type;
        float v = e.value;
        int iv = static_cast<int>(v);
        bool bv = v != 0;
        auto tex = m_textures.find(e.image);

        if (tex != m_textures.end()) {
            const Texture2D& tx = tex->second;
            const bool hover = t == "Button" && GuiGetState() != STATE_DISABLED && !GuiIsLocked() && CheckCollisionPointRec(GetMousePosition(), b);
            const Color tint = hover ? (IsMouseButtonDown(MOUSE_BUTTON_LEFT) ? ColorBrightness(e.tint, -0.25f) : ColorBrightness(e.tint, 0.15f)) : e.tint;
            DrawTexturePro(tx, {0, 0, float(tx.width), float(tx.height)}, b, {0, 0}, 0, tint);
            if (text && t == "Button") {
                const int align = GuiGetStyle(LABEL, TEXT_ALIGNMENT);
                GuiSetStyle(LABEL, TEXT_ALIGNMENT, TEXT_ALIGN_CENTER);
                GuiLabel(b, text);
                GuiSetStyle(LABEL, TEXT_ALIGNMENT, align);
            }
            if (hover && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) fire(e);
        }
        else if (t == "Window") {
            if (!e.closable) GuiPanel(b, text);
            else if (GuiWindowBox(b, text)) { e.visible = false; fire(e); }
        }
        else if (t == "Panel") GuiPanel(b, text);
        else if (t == "GroupBox") GuiGroupBox(b, text);
        else if (t == "Line") GuiLine(b, text);
        else if (t == "Label") GuiLabel(b, text);
        else if (t == "StatusBar") GuiStatusBar(b, text);
        else if (t == "Button") { if (GuiButton(b, text)) fire(e); }
        else if (t == "LabelButton") { if (GuiLabelButton(b, text)) fire(e); }
        else if (t == "Toggle") GuiToggle(b, text, &bv);
        else if (t == "CheckBox") GuiCheckBox(b, text, &bv);
        else if (t == "ToggleGroup" || t == "ComboBox") {
            const std::string items = join(e.items);
            if (t == "ComboBox") GuiComboBox(b, items.c_str(), &iv);
            else {
                const float n = static_cast<float>(std::max<std::size_t>(e.items.size(), 1));
                const float pad = static_cast<float>(GuiGetStyle(TOGGLE, GROUP_PADDING));
                GuiToggleGroup({b.x, b.y, (b.width - pad * (n - 1)) / n, b.height}, items.c_str(), &iv);
            }
        }
        else if (t == "Dropdown") m_late.push_back(&e);
        else if (t == "TextBox") {
            if (GuiTextBox(b, e.buffer.data(), static_cast<int>(e.buffer.size()), e.edit)) {
                e.edit = !e.edit;
                if (!e.edit) { e.text = e.buffer.data(); fire(e); }
            }
        }
        else if (t == "ValueBox" || t == "Spinner") {
            auto fn = t == "Spinner" ? GuiSpinner : GuiValueBox;
            if (fn(b, text, &iv, static_cast<int>(e.min), static_cast<int>(e.max), e.edit)) e.edit = !e.edit;
        }
        else if (t == "Slider" || t == "SliderBar" || t == "ProgressBar") {
            const char* right = e.showValue ? TextFormat("%.0f", v) : nullptr;
            auto fn = t == "Slider" ? GuiSlider : t == "SliderBar" ? GuiSliderBar : GuiProgressBar;
            fn(b, text, right, &v, e.min, e.max);
        }

        const float nv = (t == "Toggle" || t == "CheckBox") ? (bv ? 1.f : 0.f)
                       : (t == "ToggleGroup" || t == "ComboBox" || t == "ValueBox" || t == "Spinner") ? static_cast<float>(iv) : v;
        if (nv != e.value) {
            e.value = nv;
            fire(e);
        }
        for (auto& c : e.children) drawElement(c, scale);
    }
    GuiSetState(prevState);
}

}
