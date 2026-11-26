#pragma once
#include <raylib.h>
#include <SigmaYaml/Yaml.hpp>
#include <functional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {

// Style raygui : propriétés (couleurs par état, bordure, padding...) + extensions SigmaUi.
struct Style {
    std::vector<std::pair<int, int>> props;
    int textSize = 0;
    float radius = -1;
    std::string file, font;
};

struct Element {
    std::string type, id, text, action, style, image, tooltip;
    std::vector<std::string> items;
    Vector2 pos{}, size{};
    int anchor = 0;
    float value = 0, min = 0, max = 100;
    bool visible = true, enabled = true, closable = true, showValue = false, edit = false;
    Color tint = WHITE;
    std::vector<char> buffer;
    Rectangle bounds{};
    std::vector<Element> children;
};

// Charge "Styles:" (styles nommés) et "Theme:" (design global raygui par composant).
bool loadStyles(const std::string& path, std::string* error = nullptr);
void applyTheme(const yaml::Node& theme, const std::string& dir = {});

class Screen {
public:
    using Handler = std::function<void(Element&)>;

    Screen() = default;
    explicit Screen(const std::string& path)
    {
        std::string error;
        if (!load(path, &error)) TraceLog(LOG_WARNING, "UI: %s", error.c_str());
    }
    ~Screen();
    Screen(const Screen&) = delete;
    Screen& operator=(const Screen&) = delete;

    bool load(const std::string& path, std::string* error = nullptr);
    void draw();

    Screen& on(const std::string& action, Handler handler);
    bool triggered(const std::string& action) const { return m_fired.count(action) > 0; }

    Element* find(const std::string& id);
    float value(const std::string& id) { auto* e = find(id); return e ? e->value : 0.f; }
    std::string text(const std::string& id) { auto* e = find(id); return e ? e->text : std::string(); }
    void setValue(const std::string& id, float v) { if (auto* e = find(id)) e->value = v; }
    void setText(const std::string& id, const std::string& t);
    void setVisible(const std::string& id, bool v) { if (auto* e = find(id)) e->visible = v; }

    bool visible = true;
    Element root;

private:
    void parseElement(const yaml::Node& node, Element& out, const std::string& type);
    void layout(Element& e, Rectangle parent, float scale);
    void drawElement(Element& e, float scale);
    void fire(Element& e);

    std::string m_dir;
    Vector2 m_reference{}, m_offset{};
    int m_scaleMode = 0;
    std::unordered_map<std::string, Texture2D> m_textures;
    std::unordered_map<std::string, Handler> m_handlers;
    std::set<std::string> m_fired;
    std::vector<Element*> m_late;
    const Element* m_hit = nullptr; // seul élément interactif sous la souris (le plus haut)
};

}
