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
Elements:              # éléments posés directement sur l'écran (aucune fenêtre obligatoire)
  - Type: Image
    Image: "../tds-modern-gui-pixel-art/PNG/HUD/MONEY PANEL/Money Panel HUD.png"
    Anchor: TopRight
    Position: [-10, 10]
  - Type: Window
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

Ancien format toujours lu : une clé racine par élément (`Window:`, ou `MonNom: {Type: Image, ...}`).

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

## Thème et styles

`Theme:` règle le design raygui global, par composant ; `Default` s'applique à tous, puis chaque composant le surcharge
(`Label Button Toggle CheckBox ComboBox Dropdown TextBox ValueBox Slider ProgressBar StatusBar ScrollBar ListView`).
`Styles:` définit des styles nommés appliqués via `Style:` à un élément (et hérités par les enfants d'un conteneur).
Le thème est global à raygui : il s'applique au chargement (`ui::loadStyles()` ou `Theme:` dans un écran).

```yaml
Theme:
  Default:
    TextSize: 16
    BorderWidth: 2
    Radius: 4                      # bords arrondis, en pixels
    BorderColor: "#6b8f3a"
    BaseColor: "#1e2a1e"
    TextColor: "#d8e8c0"
    BaseColorFocused: "#2b3c2b"    # survol
    BorderColorPressed: "#e0c060"  # clic / actif
    TextColorDisabled: "#556655"   # Enabled: false
    BackgroundColor: "#141c14"
    Font: "fonts/terminal.ttf"     # optionnel, relatif au .yml
  Button:
    Radius: 8
```

| Propriété | Effet |
|---|---|
| `{Border,Base,Text}Color[Focused\|Pressed\|Disabled]` | couleurs par état (`#rrggbb` ou `#rrggbbaa`) |
| `BorderWidth`, `Radius` | épaisseur et arrondi des bordures (px) |
| `TextSize`, `TextPadding`, `TextSpacing`, `Align` | texte (`Align` : Left/Center/Right) |
| `BackgroundColor`, `LineColor` | fond des panneaux/fenêtres, lignes et GroupBox (global) |
| `Font`, `File` | police `.ttf`/`.otf`, style raygui `.rgs` (thème uniquement) |

Les arrondis passent par un hook injecté dans une copie de `raygui.h` au configure CMake (sources téléchargées intactes).

Non supporté par le parseur : ancres/alias YAML et scalaires multilignes (`|`, `>`).

## Chiffrement

Au build, `yml_crypt pack assets build/assets` copie les assets (hors `.psd`) et chiffre chaque `.yml` ; un `.yml` invalide fait échouer le build.

```bash
cmake -B build -DSIGMASIX_ASSET_KEY="ma-cle-secrete"
```

`-DSIGMASIX_ENCRYPT_ASSETS=OFF` copie les `.yml` en clair (debug). La clé est compilée dans le binaire : protection contre la lecture/modification directe, pas contre la rétro-ingénierie.
