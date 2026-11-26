#include <raylib.h>

static float g_radius = 0;

// Remplace le dessin des rectangles raygui (hook injecté dans raygui.h par CMake) : rayon en pixels.
static bool roundedRectangle(Rectangle r, int border, Color borderColor, Color color)
{
    const float side = r.width < r.height ? r.width : r.height;
    if (g_radius <= 0 || side <= 0) return false;
    const float roundness = 2 * g_radius / side > 1 ? 1 : 2 * g_radius / side;
    if (color.a) DrawRectangleRounded(r, roundness, 8, color);
    if (border > 0 && borderColor.a) DrawRectangleRoundedLinesEx(r, roundness, 8, static_cast<float>(border), borderColor);
    return true;
}

#define RAYGUI_DRAW_RECTANGLE_HOOK roundedRectangle
#define RAYGUI_IMPLEMENTATION
#include <raygui.h>
#include <SigmaUi/Ui.hpp>
#include <algorithm>
#include <tuple>

namespace ui {

namespace {

std::unordered_map<std::string, Style> g_styles;
float g_themeRadius[RAYGUI_MAX_CONTROLS] = {0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
float g_styleRadius = -1;
int g_textSize = 10;
Font g_font{};

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

int control(const std::string& t)
{
    static const std::unordered_map<std::string, int> controls = {
        {"Default", DEFAULT}, {"Label", LABEL}, {"Button", BUTTON}, {"LabelButton", BUTTON}, {"Toggle", TOGGLE},
        {"ToggleGroup", TOGGLE}, {"CheckBox", CHECKBOX}, {"ComboBox", COMBOBOX}, {"Dropdown", DROPDOWNBOX},
        {"TextBox", TEXTBOX}, {"ValueBox", VALUEBOX}, {"Spinner", VALUEBOX}, {"Slider", SLIDER}, {"SliderBar", SLIDER},
        {"ProgressBar", PROGRESSBAR}, {"StatusBar", STATUSBAR}, {"ScrollBar", SCROLLBAR}, {"ListView", LISTVIEW}};
    auto it = controls.find(t);
    return it == controls.end() ? -1 : it->second;
}

int property(const std::string& key)
{
    static const auto props = [] {
        std::unordered_map<std::string, int> m = {
            {"BorderWidth", BORDER_WIDTH}, {"TextPadding", TEXT_PADDING}, {"Align", TEXT_ALIGNMENT},
            {"TextSpacing", TEXT_SPACING}, {"LineColor", LINE_COLOR}, {"BackgroundColor", BACKGROUND_COLOR},
            {"TextLineSpacing", TEXT_LINE_SPACING}};
        const char* parts[] = {"Border", "Base", "Text"};
        const char* states[] = {"", "Focused", "Pressed", "Disabled"};
        for (int st = 0; st < 4; ++st)
            for (int p = 0; p < 3; ++p) m[std::string(parts[p]) + "Color" + states[st]] = p + 3 * st;
        return m;
    }();
    auto it = props.find(key);
    return it == props.end() ? -1 : it->second;
}

Style parseStyle(const yaml::Node& n)
{
    Style s;
    for (std::size_t i = 0; i < n.size() && n.isMap(); ++i) {
        const std::string& key = n.keys()[i];
        const yaml::Node& v = n.items()[i];
        if (key == "TextSize") s.textSize = static_cast<int>(v.asInt());
        else if (key == "Radius") s.radius = v.asFloat();
        else if (key == "File") s.file = v.asString();
        else if (key == "Font") s.font = v.asString();
        else if (const int p = property(key); p >= 0) {
            const std::string a = v.asString();
            const int value = key.find("Color") != std::string::npos ? color(v)
                            : p == TEXT_ALIGNMENT ? (a == "Left" ? 0 : a == "Right" ? 2 : a == "Center" ? 1 : static_cast<int>(v.asInt(1)))
                            : static_cast<int>(v.asInt());
            s.props.emplace_back(p, value);
        }
    }
    return s;
}

void readStyles(const yaml::Node& styles)
{
    for (std::size_t i = 0; i < styles.size() && styles.isMap(); ++i)
        g_styles[styles.keys()[i]] = parseStyle(styles.items()[i]);
}

// Applique un style le temps d'un dessin (enfants inclus) puis restaure l'état raygui.
class StyleScope {
public:
    StyleScope(const Element& e, float scale)
    {
        auto it = g_styles.find(e.style);
        const Style* s = it == g_styles.end() ? nullptr : &it->second;
        const int c = std::max(control(e.type), 0);
        if (s && s->textSize) g_textSize = s->textSize;
        if (s && s->radius >= 0) g_styleRadius = s->radius;
        g_radius = scale * (g_styleRadius >= 0 ? g_styleRadius : g_themeRadius[c] >= 0 ? g_themeRadius[c] : g_themeRadius[DEFAULT]);
        set(DEFAULT, TEXT_SIZE, static_cast<int>(g_textSize * scale + 0.5f));
        if (s)
            for (auto [p, v] : s->props) set(p >= RAYGUI_MAX_PROPS_BASE ? DEFAULT : c, p, v);
    }
    ~StyleScope()
    {
        g_textSize = m_textSize;
        g_styleRadius = m_styleRadius;
        g_radius = m_radius;
        for (auto it = m_saved.rbegin(); it != m_saved.rend(); ++it) GuiSetStyle(std::get<0>(*it), std::get<1>(*it), std::get<2>(*it));
    }

private:
    // DEFAULT se propage à tous les contrôles : on sauvegarde chacun pour restaurer le thème intact.
    void set(int c, int p, int v)
    {
        if (c == DEFAULT && p < RAYGUI_MAX_PROPS_BASE)
            for (int k = 1; k < RAYGUI_MAX_CONTROLS; ++k) m_saved.emplace_back(k, p, GuiGetStyle(k, p));
        m_saved.emplace_back(c, p, GuiGetStyle(c, p));
        GuiSetStyle(c, p, v);
    }
    int m_textSize = g_textSize;
    float m_styleRadius = g_styleRadius, m_radius = g_radius;
    std::vector<std::tuple<int, int, int>> m_saved;
};

std::string join(const std::vector<std::string>& items)
{
    std::string out;
    for (const auto& s : items) out += (out.empty() ? "" : ";") + s;
    return out;
}

}

void applyTheme(const yaml::Node& theme, const std::string& dir)
{
    auto apply = [&](const std::string& name, const yaml::Node& node) {
        const int c = control(name);
        if (c < 0) return TraceLog(LOG_WARNING, "UI: composant de thème inconnu '%s'", name.c_str());
        const Style s = parseStyle(node);
        if (!s.file.empty()) GuiLoadStyle((dir + s.file).c_str());
        for (auto [p, v] : s.props) GuiSetStyle(p >= RAYGUI_MAX_PROPS_BASE ? DEFAULT : c, p, v);
        if (s.textSize) GuiSetStyle(DEFAULT, TEXT_SIZE, s.textSize);
        if (s.radius >= 0) g_themeRadius[c] = s.radius;
        if (!s.font.empty()) {
            if (g_font.texture.id) UnloadFont(g_font);
            g_font = LoadFontEx((dir + s.font).c_str(), GuiGetStyle(DEFAULT, TEXT_SIZE) * 2, nullptr, 0);
            GuiSetFont(g_font);
        }
    };
    if (theme.has("Default")) apply("Default", theme["Default"]);
    for (std::size_t i = 0; i < theme.size() && theme.isMap(); ++i)
        if (theme.keys()[i] != "Default") apply(theme.keys()[i], theme.items()[i]);
}

bool loadStyles(const std::string& path, std::string* error)
{
    yaml::Node doc;
    if (!yaml::loadFile(path, doc, error)) return false;
    const auto slash = path.find_last_of("/\\");
    const std::string dir = slash == std::string::npos ? "" : path.substr(0, slash + 1);
    if (doc.has("Theme")) applyTheme(doc["Theme"], dir);
    if (doc.has("Styles") || !doc.has("Theme")) readStyles(doc.has("Styles") ? doc["Styles"] : doc);
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
        else if (key == "Theme") applyTheme(node, m_dir);
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
