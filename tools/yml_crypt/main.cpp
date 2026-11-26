#include <SigmaYaml/Yaml.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>

namespace fs = std::filesystem;

namespace {

int usage()
{
    std::cerr << "usage :\n"
                 "  yml_crypt encrypt <entree.yml> <sortie.yml> [--key <cle>]\n"
                 "  yml_crypt decrypt <entree.yml> <sortie.yml> [--key <cle>]\n"
                 "  yml_crypt pack    <dossier_src> <dossier_dst> [--key <cle>]\n";
    return 2;
}

bool readFile(const fs::path& path, std::string& out)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

bool writeFile(const fs::path& path, const std::string& data)
{
    if (path.has_parent_path()) fs::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(file);
}

bool encryptFile(const fs::path& in, const fs::path& out)
{
    std::string text;
    if (!readFile(in, text)) {
        std::cerr << "erreur : lecture de " << in << '\n';
        return false;
    }
    if (yaml::Cipher::isEncrypted(text)) return writeFile(out, text);

    std::string error;
    yaml::Node node;
    if (!yaml::parse(text, node, &error)) {
        std::cerr << in.string() << " : " << error << '\n';
        return false;
    }
    return writeFile(out, yaml::Cipher::encrypt(text));
}

bool decryptFile(const fs::path& in, const fs::path& out)
{
    std::string data, plain;
    if (!readFile(in, data)) return false;
    if (!yaml::Cipher::isEncrypted(data)) return writeFile(out, data);
    if (!yaml::Cipher::decrypt(data, plain)) {
        std::cerr << "erreur : déchiffrement de " << in << " (clé invalide ?)\n";
        return false;
    }
    return writeFile(out, plain);
}

bool pack(const fs::path& src, const fs::path& dst)
{
    if (!fs::is_directory(src)) {
        std::cerr << "erreur : dossier introuvable " << src << '\n';
        return false;
    }
    bool ok = true;
    int count = 0;
    for (const auto& entry : fs::recursive_directory_iterator(src)) {
        if (!entry.is_regular_file() || entry.path().extension() == ".psd") continue;
        const fs::path target = dst / fs::relative(entry.path(), src);
        const std::string ext = entry.path().extension().string();
        if (ext == ".yml" || ext == ".yaml") {
            ok &= encryptFile(entry.path(), target);
            ++count;
        } else {
            fs::create_directories(target.parent_path());
            fs::copy_file(entry.path(), target, fs::copy_options::update_existing);
        }
    }
    std::cout << "yml_crypt : " << count << " fichier(s) yml chiffré(s) -> " << dst.string() << '\n';
    return ok;
}

}

int main(int argc, char** argv)
{
    if (argc < 4) return usage();
    for (int i = 4; i + 1 < argc; i += 2)
        if (std::string(argv[i]) == "--key") yaml::Cipher::setKey(argv[i + 1]);

    const std::string command = argv[1];
    try {
        if (command == "encrypt") return encryptFile(argv[2], argv[3]) ? 0 : 1;
        if (command == "decrypt") return decryptFile(argv[2], argv[3]) ? 0 : 1;
        if (command == "pack") return pack(argv[2], argv[3]) ? 0 : 1;
    } catch (const std::exception& e) {
        std::cerr << "erreur : " << e.what() << '\n';
        return 1;
    }
    return usage();
}
