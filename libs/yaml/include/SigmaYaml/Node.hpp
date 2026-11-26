#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace yaml {

enum class NodeType : std::uint8_t { Null, Scalar, Sequence, Map };

// Noeud YAML : scalaire (texte brut), séquence ou map ordonnée.
class Node {
public:
    Node() = default;
    Node(const char* value);
    Node(std::string value);
    Node(bool value);
    Node(int value);
    Node(std::int64_t value);
    Node(float value);
    Node(double value);

    static Node map();
    static Node sequence();
    static const Node& null();

    NodeType type() const { return m_type; }
    bool isNull() const { return m_type == NodeType::Null; }
    bool isScalar() const { return m_type == NodeType::Scalar; }
    bool isSequence() const { return m_type == NodeType::Sequence; }
    bool isMap() const { return m_type == NodeType::Map; }
    std::size_t size() const { return m_items.size(); }

    bool has(const std::string& key) const { return find(key) != nullptr; }
    const Node* find(const std::string& key) const;
    Node* find(const std::string& key);
    bool remove(const std::string& key);

    Node& operator[](const std::string& key);
    const Node& operator[](const std::string& key) const;
    Node& operator[](const char* key) { return (*this)[std::string(key)]; }
    const Node& operator[](const char* key) const { return (*this)[std::string(key)]; }
    Node& operator[](std::size_t index);
    const Node& operator[](std::size_t index) const;
    Node& operator[](int index) { return (*this)[static_cast<std::size_t>(index)]; }
    const Node& operator[](int index) const { return (*this)[static_cast<std::size_t>(index)]; }

    Node& push(Node node);
    void clear();

    const std::string& raw() const { return m_value; }
    std::string asString(const std::string& def = {}) const;
    std::int64_t asInt(std::int64_t def = 0) const;
    double asDouble(double def = 0.0) const;
    float asFloat(float def = 0.f) const { return static_cast<float>(asDouble(def)); }
    bool asBool(bool def = false) const;
    std::vector<std::string> asStrings() const;
    std::vector<float> asFloats() const;

    const std::vector<std::string>& keys() const { return m_keys; }
    const std::vector<Node>& items() const { return m_items; }
    std::vector<Node>& items() { return m_items; }

private:
    NodeType m_type = NodeType::Null;
    std::string m_value;
    std::vector<std::string> m_keys;
    std::vector<Node> m_items;
};

}
