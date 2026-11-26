add_library(SigmaYaml STATIC src/Node.cpp src/Yaml.cpp src/Cipher.cpp)
target_include_directories(SigmaYaml PUBLIC include)
target_compile_features(SigmaYaml PUBLIC cxx_std_17)

set(SIGMASIX_ASSET_KEY "SigmaSix-default-asset-key" CACHE STRING "Clé de chiffrement des assets .yml")
target_compile_definitions(SigmaYaml PRIVATE SIGMA_YAML_KEY="${SIGMASIX_ASSET_KEY}")
