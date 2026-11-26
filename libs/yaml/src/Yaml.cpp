#include <SigmaYaml/Yaml.hpp>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <sstream>

namespace yaml {

namespace {

struct Line {
    int indent;
    std::string text;
    int number;
};

std::string trim(const std::string& s)
{
    const std::size_t begin = s.find_first_not_of(" \t\r");
    if (begin == std::string::npos) return {};
    return s.substr(begin, s.find_last_not_of(" \t\r") - begin + 1);
}

bool opensQuote(const std::string& s, std::size_t i)
{
    if (i == 0) return true;
    const char prev = s[i - 1];
    return prev == ' ' || prev == '[' || prev == '{' || prev == ',' || prev == ':' || prev == '-';
}

std::string stripComment(const std::string& line)
{
    char quote = 0;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (quote) {
            if (quote == '"' && c == '\\') ++i;
            else if (c == quote) quote = 0;
        } else if ((c == '"' || c == '\'') && opensQuote(line, i)) {
            quote = c;
        } else if (c == '#' && (i == 0 || line[i - 1] == ' ' || line[i - 1] == '\t')) {
            return line.substr(0, i);
        }
    }
    return line;
}

bool isSeqItem(const std::string& text)
{
    return text == "-" || (text.size() > 1 && text[0] == '-' && text[1] == ' ');
}

std::size_t findKeyColon(const std::string& text)
{
    if (text.empty() || text[0] == '[' || text[0] == '{') return std::string::npos;
    char quote = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (quote) {
            if (quote == '"' && c == '\\') ++i;
            else if (c == quote) quote = 0;
        } else if ((c == '"' || c == '\'') && i == 0) {
            quote = c;
        } else if (c == ':' && (i + 1 == text.size() || text[i + 1] == ' ')) {
            return i;
        }
    }
    return std::string::npos;
}

void appendUtf8(std::string& out, std::uint32_t cp)
{
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

// Valeurs sur une ligne : scalaires, "chaînes", [séquences], {maps}.
class InlineParser {
public:
    InlineParser(const std::string& text, int line) : m_text(text), m_line(line) {}

    Node parseValue()
    {
        Node node = value(false);
        skipSpaces();
        if (m_pos != m_text.size()) throw Error("caractères inattendus après la valeur", m_line);
        return node;
    }

    std::string parseKey()
    {
        skipSpaces();
        if (peek() != '"' && peek() != '\'') return trim(m_text);
        std::string key = quoted();
        skipSpaces();
        if (m_pos != m_text.size()) throw Error("clé invalide", m_line);
        return key;
    }

private:
    char peek() const { return m_pos < m_text.size() ? m_text[m_pos] : '\0'; }

    void skipSpaces()
    {
        while (m_pos < m_text.size() && (m_text[m_pos] == ' ' || m_text[m_pos] == '\t')) ++m_pos;
    }

    void expect(char c)
    {
        skipSpaces();
        if (peek() != c) throw Error(std::string("'") + c + "' attendu", m_line);
        ++m_pos;
    }

    Node value(bool inFlow)
    {
        skipSpaces();
        const char c = peek();
        if (c == '[') return flowSequence();
        if (c == '{') return flowMap();
        if (c == '"' || c == '\'') return Node(quoted());
        const std::string text = plain(inFlow, false);
        if (text.empty() || text == "~" || text == "null" || text == "Null" || text == "NULL") return {};
        return Node(text);
    }

    std::string plain(bool inFlow, bool isKey)
    {
        const std::size_t begin = m_pos;
        for (; m_pos < m_text.size(); ++m_pos) {
            const char c = m_text[m_pos];
            if (inFlow && (c == ',' || c == ']' || c == '}')) break;
            if (isKey && c == ':') break;
        }
        return trim(m_text.substr(begin, m_pos - begin));
    }

    std::string quoted()
    {
        const char quote = m_text[m_pos++];
        std::string out;
        while (m_pos < m_text.size()) {
            const char c = m_text[m_pos++];
            if (quote == '\'') {
                if (c != '\'') { out += c; continue; }
                if (peek() != '\'') return out;
                out += '\'';
                ++m_pos;
                continue;
            }
            if (c == '"') return out;
            if (c != '\\') { out += c; continue; }
            if (m_pos >= m_text.size()) break;
            switch (const char esc = m_text[m_pos++]) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case '0': out += '\0'; break;
                case 'u':
                    if (m_pos + 4 > m_text.size()) throw Error("séquence \\u invalide", m_line);
                    appendUtf8(out, static_cast<std::uint32_t>(std::stoul(m_text.substr(m_pos, 4), nullptr, 16)));
                    m_pos += 4;
                    break;
                default: out += esc; break;
            }
        }
        throw Error("chaîne non terminée", m_line);
    }

    Node flowSequence()
    {
        ++m_pos;
        Node seq = Node::sequence();
        skipSpaces();
        if (peek() == ']') { ++m_pos; return seq; }
        for (;;) {
            seq.push(value(true));
            skipSpaces();
            if (peek() == ',') { ++m_pos; continue; }
            expect(']');
            return seq;
        }
    }

    Node flowMap()
    {
        ++m_pos;
        Node map = Node::map();
        skipSpaces();
        if (peek() == '}') { ++m_pos; return map; }
        for (;;) {
            skipSpaces();
            const std::string key = (peek() == '"' || peek() == '\'') ? quoted() : plain(true, true);
            expect(':');
            map[key] = value(true);
            skipSpaces();
            if (peek() == ',') { ++m_pos; continue; }
            expect('}');
            return map;
        }
    }

    const std::string& m_text;
    int m_line;
    std::size_t m_pos = 0;
};

// Structure par indentation : maps, séquences "- ", imbrications.
class BlockParser {
public:
    explicit BlockParser(const std::string& text)
    {
        std::istringstream stream(text);
        std::string raw;
        for (int number = 1; std::getline(stream, raw); ++number) {
            if (number == 1 && raw.rfind("\xEF\xBB\xBF", 0) == 0) raw.erase(0, 3);
            const std::string line = stripComment(raw);
            const std::size_t indent = line.find_first_not_of(' ');
            if (indent == std::string::npos) continue;
            if (line[indent] == '\t') throw Error("tabulation interdite dans l'indentation", number);
            std::string content = trim(line.substr(indent));
            if (content.empty() || (indent == 0 && (content == "---" || content == "..."))) continue;
            m_lines.push_back({static_cast<int>(indent), std::move(content), number});
        }
    }

    Node parse()
    {
        if (m_lines.empty()) return {};
        Node root = block(m_lines[0].indent);
        if (m_idx < m_lines.size()) throw Error("indentation incohérente", m_lines[m_idx].number);
        return root;
    }

private:
    Node block(int indent)
    {
        const Line& line = m_lines[m_idx];
        if (isSeqItem(line.text)) return sequence(indent);
        if (findKeyColon(line.text) != std::string::npos) return map(indent);
        ++m_idx;
        return InlineParser(line.text, line.number).parseValue();
    }

    Node nested(int parentIndent, bool allowSameIndentSeq)
    {
        if (m_idx >= m_lines.size()) return {};
        const Line& next = m_lines[m_idx];
        if (next.indent > parentIndent) return block(next.indent);
        if (allowSameIndentSeq && next.indent == parentIndent && isSeqItem(next.text)) return sequence(parentIndent);
        return {};
    }

    Node sequence(int indent)
    {
        Node seq = Node::sequence();
        while (m_idx < m_lines.size()) {
            Line& line = m_lines[m_idx];
            if (line.indent < indent || !isSeqItem(line.text)) break;
            if (line.indent > indent) throw Error("indentation inattendue", line.number);

            const std::size_t start = line.text.find_first_not_of(' ', 1);
            if (start == std::string::npos) {
                ++m_idx;
                seq.push(nested(indent, false));
                continue;
            }
            const std::string content = line.text.substr(start);
            if (isSeqItem(content) || findKeyColon(content) != std::string::npos) {
                line.indent = indent + static_cast<int>(start);
                line.text = content;
                seq.push(block(line.indent));
                continue;
            }
            ++m_idx;
            seq.push(InlineParser(content, line.number).parseValue());
        }
        return seq;
    }

    Node map(int indent)
    {
        Node result = Node::map();
        while (m_idx < m_lines.size()) {
            const Line& line = m_lines[m_idx];
            if (line.indent < indent || isSeqItem(line.text)) break;
            if (line.indent > indent) throw Error("indentation inattendue", line.number);

            const std::size_t colon = findKeyColon(line.text);
            if (colon == std::string::npos) throw Error("'clé: valeur' attendu", line.number);
            const int number = line.number;
            const std::string key = InlineParser(line.text.substr(0, colon), number).parseKey();
            const std::string rest = trim(line.text.substr(colon + 1));
            if (result.has(key)) throw Error("clé dupliquée '" + key + "'", number);
            ++m_idx;

            if (rest.empty()) result[key] = nested(indent, true);
            else if (rest[0] == '|' || rest[0] == '>') throw Error("scalaires multilignes non supportés", number);
            else result[key] = InlineParser(rest, number).parseValue();
        }
        return result;
    }

    std::vector<Line> m_lines;
    std::size_t m_idx = 0;
};

bool needsQuotes(const std::string& v)
{
    if (v.empty() || v == "~" || v == "null" || v == "-") return true;
    if (v.front() == ' ' || v.back() == ' ' || v.back() == ':') return true;
    if (v.size() > 1 && v[0] == '-' && v[1] == ' ') return true;
    if (std::string("?:,[]{}#&*!|>'\"%@`").find(v.front()) != std::string::npos) return true;
    for (const char c : v)
        if (static_cast<unsigned char>(c) < 0x20 || std::string(",[]{}\"").find(c) != std::string::npos) return true;
    return v.find(": ") != std::string::npos || v.find(" #") != std::string::npos;
}

std::string quote(const std::string& v)
{
    if (!needsQuotes(v)) return v;
    std::string out = "\"";
    for (const char c : v) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(c));
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    return out + "\"";
}

bool isInline(const Node& node)
{
    if (node.isNull() || node.isScalar() || node.size() == 0) return true;
    if (node.isMap()) return false;
    std::size_t length = 0;
    for (const Node& item : node.items()) {
        if (!item.isScalar() && !item.isNull()) return false;
        length += item.raw().size() + 2;
    }
    return length <= 80;
}

std::string inlineText(const Node& node)
{
    switch (node.type()) {
        case NodeType::Null: return "~";
        case NodeType::Scalar: return quote(node.raw());
        case NodeType::Map: return "{}";
        case NodeType::Sequence: {
            std::string out = "[";
            for (std::size_t i = 0; i < node.size(); ++i) out += (i ? ", " : "") + inlineText(node[i]);
            return out + "]";
        }
    }
    return {};
}

void dumpNode(const Node& node, int indent, std::string& out)
{
    const std::string pad(static_cast<std::size_t>(indent), ' ');
    if (node.isMap()) {
        for (std::size_t i = 0; i < node.size(); ++i) {
            const Node& child = node.items()[i];
            out += pad + quote(node.keys()[i]) + ":";
            if (isInline(child)) {
                out += " " + inlineText(child) + "\n";
            } else {
                out += "\n";
                dumpNode(child, indent + 2, out);
            }
        }
    } else if (node.isSequence()) {
        for (const Node& item : node.items()) {
            if (isInline(item)) {
                out += pad + "- " + inlineText(item) + "\n";
                continue;
            }
            std::string child;
            dumpNode(item, indent + 2, child);
            child.replace(0, static_cast<std::size_t>(indent + 2), pad + "- ");
            out += child;
        }
    } else {
        out += pad + inlineText(node) + "\n";
    }
}

}

Node parse(const std::string& text)
{
    return BlockParser(text).parse();
}

bool parse(const std::string& text, Node& out, std::string* error)
{
    try {
        out = parse(text);
        return true;
    } catch (const std::exception& e) {
        if (error) *error = e.what();
        return false;
    }
}

std::string dump(const Node& node)
{
    std::string out;
    dumpNode(node, 0, out);
    return out;
}

bool loadFile(const std::string& path, Node& out, std::string* error)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        if (error) *error = "impossible d'ouvrir " + path;
        return false;
    }
    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (Cipher::isEncrypted(data)) {
        std::string plain;
        if (!Cipher::decrypt(data, plain)) {
            if (error) *error = "déchiffrement impossible (clé invalide ou fichier corrompu) : " + path;
            return false;
        }
        data = std::move(plain);
    }
    if (parse(data, out, error)) return true;
    if (error) *error = path + " : " + *error;
    return false;
}

bool saveFile(const std::string& path, const Node& node, bool encrypted, std::string* error)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        if (error) *error = "impossible d'écrire " + path;
        return false;
    }
    const std::string text = dump(node);
    const std::string data = encrypted ? Cipher::encrypt(text) : text;
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(file);
}

}
