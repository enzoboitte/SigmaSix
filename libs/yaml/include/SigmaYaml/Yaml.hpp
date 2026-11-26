#pragma once
#include <SigmaYaml/Cipher.hpp>
#include <SigmaYaml/Node.hpp>
#include <stdexcept>
#include <string>

namespace yaml {

class Error : public std::runtime_error {
public:
    Error(const std::string& message, int line)
        : std::runtime_error("ligne " + std::to_string(line) + " : " + message), line(line) {}
    int line;
};

Node parse(const std::string& text);
bool parse(const std::string& text, Node& out, std::string* error = nullptr);
std::string dump(const Node& node);

// Lecture transparente : détecte automatiquement un fichier chiffré.
bool loadFile(const std::string& path, Node& out, std::string* error = nullptr);
bool saveFile(const std::string& path, const Node& node, bool encrypted = false, std::string* error = nullptr);

}
