#include <SigmaYaml/Node.hpp>
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace yaml {

namespace {

std::string formatDouble(double value)
{
    std::ostringstream stream;
    stream.precision(9);
    stream << value;
    return stream.str();
}

}

Node::Node(const char* value) : m_type(NodeType::Scalar), m_value(value ? value : "") {}
Node::Node(std::string value) : m_type(NodeType::Scalar), m_value(std::move(value)) {}
Node::Node(bool value) : m_type(NodeType::Scalar), m_value(value ? "true" : "false") {}
Node::Node(int value) : m_type(NodeType::Scalar), m_value(std::to_string(value)) {}
Node::Node(std::int64_t value) : m_type(NodeType::Scalar), m_value(std::to_string(value)) {}
Node::Node(float value) : m_type(NodeType::Scalar), m_value(formatDouble(value)) {}
Node::Node(double value) : m_type(NodeType::Scalar), m_value(formatDouble(value)) {}

Node Node::map()
{
    Node node;
    node.m_type = NodeType::Map;
    return node;
}

Node Node::sequence()
{
    Node node;
    node.m_type = NodeType::Sequence;
    return node;
}

const Node& Node::null()
{
    static const Node instance;
    return instance;
}

const Node* Node::find(const std::string& key) const
{
    if (m_type != NodeType::Map) return nullptr;
    auto it = std::find(m_keys.begin(), m_keys.end(), key);
    return it == m_keys.end() ? nullptr : &m_items[static_cast<std::size_t>(it - m_keys.begin())];
}

Node* Node::find(const std::string& key)
{
    return const_cast<Node*>(static_cast<const Node*>(this)->find(key));
}

bool Node::remove(const std::string& key)
{
    auto it = std::find(m_keys.begin(), m_keys.end(), key);
    if (it == m_keys.end()) return false;
    m_items.erase(m_items.begin() + (it - m_keys.begin()));
    m_keys.erase(it);
    return true;
}

Node& Node::operator[](const std::string& key)
{
    if (m_type == NodeType::Null) m_type = NodeType::Map;
    if (m_type != NodeType::Map) throw std::logic_error("yaml::Node : accès par clé sur un noeud qui n'est pas une map");
    if (Node* node = find(key)) return *node;
    m_keys.push_back(key);
    return m_items.emplace_back();
}

const Node& Node::operator[](const std::string& key) const
{
    const Node* node = find(key);
    return node ? *node : null();
}

Node& Node::operator[](std::size_t index)
{
    if (m_type == NodeType::Null) m_type = NodeType::Sequence;
    if (m_type != NodeType::Sequence) throw std::logic_error("yaml::Node : accès par index sur un noeud qui n'est pas une séquence");
    if (index >= m_items.size()) m_items.resize(index + 1);
    return m_items[index];
}

const Node& Node::operator[](std::size_t index) const
{
    return (m_type == NodeType::Sequence && index < m_items.size()) ? m_items[index] : null();
}

Node& Node::push(Node node)
{
    if (m_type == NodeType::Null) m_type = NodeType::Sequence;
    if (m_type != NodeType::Sequence) throw std::logic_error("yaml::Node : push sur un noeud qui n'est pas une séquence");
    return m_items.emplace_back(std::move(node));
}

void Node::clear()
{
    m_type = NodeType::Null;
    m_value.clear();
    m_keys.clear();
    m_items.clear();
}

std::string Node::asString(const std::string& def) const
{
    return m_type == NodeType::Scalar ? m_value : def;
}

std::int64_t Node::asInt(std::int64_t def) const
{
    if (m_type != NodeType::Scalar || m_value.empty()) return def;
    const char* begin = m_value.data();
    const char* end = begin + m_value.size();
    if (*begin == '+') ++begin;
    std::int64_t value = 0;
    auto [ptr, err] = std::from_chars(begin, end, value);
    if (err == std::errc() && ptr == end) return value;
    return static_cast<std::int64_t>(asDouble(static_cast<double>(def)));
}

double Node::asDouble(double def) const
{
    if (m_type != NodeType::Scalar || m_value.empty()) return def;
    char* end = nullptr;
    const double value = std::strtod(m_value.c_str(), &end);
    return (end && *end == '\0') ? value : def;
}

bool Node::asBool(bool def) const
{
    if (m_type != NodeType::Scalar) return def;
    std::string v = m_value;
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (v == "true" || v == "yes" || v == "on" || v == "1") return true;
    if (v == "false" || v == "no" || v == "off" || v == "0") return false;
    return def;
}

std::vector<std::string> Node::asStrings() const
{
    std::vector<std::string> values;
    if (m_type == NodeType::Scalar) values.push_back(m_value);
    else if (m_type == NodeType::Sequence)
        for (const Node& item : m_items) values.push_back(item.asString());
    return values;
}

std::vector<float> Node::asFloats() const
{
    std::vector<float> values;
    if (m_type == NodeType::Scalar) values.push_back(asFloat());
    else if (m_type == NodeType::Sequence)
        for (const Node& item : m_items) values.push_back(item.asFloat());
    return values;
}

}
