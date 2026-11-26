# SigmaUi · SigmaYaml

- **SigmaYaml** (`libs/yaml`) : lecture/écriture YAML légère + chiffrement ChaCha20 (format binaire `SSYE`).
  `yaml::loadFile` détecte seul si le fichier est chiffré.
- **SigmaUi** (`libs/ui`) : interfaces raygui décrites en `.yml`.
- **yml_crypt** (`tools/yml_crypt`) : chiffre les `.yml` des assets au build.
- **Éditeur web** (`tools/ui-editor/index.html`) : à ouvrir directement dans le navigateur, génère le `.yml` en clair.

## Utilisation

```cpp
#include <SigmaUi/Ui.hpp>

ui::loadStyles("assets/ui/styles.yml");
ui::Screen menu("assets/ui/menu.yml");
menu.on("StartCampaign", [](ui::Element&) { /* ... */ })
    .on("Volume", [](ui::Element& e) { SetMasterVolume(e.value / 100.f); });

// boucle, entre BeginDrawing() et EndDrawing()
menu.draw();
if (menu.triggered("OpenSettings")) { /* ... */ }
float volume = menu.value("volume");
```

## Format

```yaml
Screen:                # optionnel : résolution de référence
  Size: [800, 600]
  Scale: Fit           # None | Fit (letterbox) | Expand
Styles:                # optionnel : sinon ui::loadStyles()
  Tactical: {TextSize: 16, BaseColor: "#1e2a1e", BorderColor: "#6b8f3a", TextColor: "#d8e8c0", BackgroundColor: "#141c14"}
Window:                # chaque clé racine = un élément (Type = la clé, ou "Type:")
  Title: "SigmaSix"
  Anchor: Center
  Size: [400, 300]
  Style: "Tactical"
  Children:
    - Type: Button
      Text: "CAMPAIGN"
      Position: [120, 120]
      Size: [160, 40]
      Action: "StartCampaign"
```

| Clé | Rôle |
|---|---|
| `Type` | `Window Panel GroupBox Container Line Label Button LabelButton Toggle ToggleGroup CheckBox ComboBox Dropdown TextBox ValueBox Spinner Slider SliderBar ProgressBar StatusBar Image` |
| `Position`, `Size` | relatifs au parent ; `Size` absent = taille auto |
| `Anchor` | `TopLeft Top TopRight Left Center Right BottomLeft Bottom BottomRight` (point d'ancrage dans le parent et pivot de l'élément) |
| `Text` / `Title`, `Items` | textes ; `Items` pour ToggleGroup, ComboBox, Dropdown |
| `Value`, `Min`, `Max`, `Checked`, `ShowValue`, `MaxLength` | valeurs des contrôles |
| `Action` | déclenchée au clic ou au changement de valeur (`on()` / `triggered()`) |
| `Id` | accès par `find()`, `value()`, `text()`, `setText()`, `setVisible()` |
| `Style` | style nommé (hérité par les enfants d'un conteneur) |
| `Image`, `Tint` | texture relative au `.yml` (Image, ou Button skinné) |
| `Tooltip`, `Visible`, `Enabled`, `Closable` | divers |

Styles : `TextSize`, `Align` (Left/Center/Right), `TextColor`, `BaseColor`, `BorderColor`, `BackgroundColor`, `File` (`.rgs` raygui).

Non supporté par le parseur : ancres/alias YAML et scalaires multilignes (`|`, `>`).

## Chiffrement

Au build, `yml_crypt pack assets build/assets` copie les assets (hors `.psd`) et chiffre chaque `.yml` ; un `.yml` invalide fait échouer le build.

```bash
cmake -B build -DSIGMASIX_ASSET_KEY="ma-cle-secrete"
```

`-DSIGMASIX_ENCRYPT_ASSETS=OFF` copie les `.yml` en clair (debug). La clé est compilée dans le binaire : protection contre la lecture/modification directe, pas contre la rétro-ingénierie.
