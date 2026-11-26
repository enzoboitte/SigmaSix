// Schéma des éléments, mise en page (identique à SigmaUi) et conversion YAML <-> modèle.
const Model = (() => {
  const ANCHORS = ['TopLeft', 'Top', 'TopRight', 'Left', 'Center', 'Right', 'BottomLeft', 'Bottom', 'BottomRight'];
  const CONTAINERS = ['Window', 'Panel', 'GroupBox', 'Container'];
  const SLIDERS = ['Slider', 'SliderBar', 'ProgressBar'];
  const LISTS = ['ToggleGroup', 'ComboBox', 'Dropdown'];
  const NUMBERS = ['ValueBox', 'Spinner', ...SLIDERS];
  const ACTIONS = ['Window', 'Button', 'LabelButton', 'Toggle', 'CheckBox', 'TextBox', 'Image', ...LISTS, ...NUMBERS];

  const TYPES = {
    Window: { Size: [320, 240], Title: 'Fenêtre' },
    Panel: { Size: [200, 150] },
    GroupBox: { Size: [200, 120], Text: 'Groupe' },
    Container: { Size: [200, 150] },
    Label: { Text: 'Label' },
    Button: { Size: [120, 30], Text: 'Bouton' },
    LabelButton: { Size: [120, 24], Text: 'Lien' },
    Toggle: { Size: [120, 30], Text: 'Toggle' },
    ToggleGroup: { Size: [240, 30], Items: ['A', 'B', 'C'] },
    CheckBox: { Size: [20, 20], Text: 'Option' },
    ComboBox: { Size: [160, 30], Items: ['Un', 'Deux'] },
    Dropdown: { Size: [160, 30], Items: ['Un', 'Deux'] },
    TextBox: { Size: [200, 30], Text: '' },
    ValueBox: { Size: [120, 30], Min: 0, Max: 100 },
    Spinner: { Size: [120, 30], Min: 0, Max: 100 },
    Slider: { Size: [160, 20], Min: 0, Max: 100, Value: 50 },
    SliderBar: { Size: [160, 20], Min: 0, Max: 100, Value: 50 },
    ProgressBar: { Size: [160, 20], Min: 0, Max: 100, Value: 50 },
    StatusBar: { Size: [300, 24], Text: 'Statut' },
    Line: { Size: [200, 12] },
    Image: { Size: [64, 64], Image: '' },
  };

  const THEME_KEYS = ['Default', 'Label', 'Button', 'Toggle', 'CheckBox', 'ComboBox', 'Dropdown', 'TextBox', 'ValueBox', 'Slider', 'ProgressBar', 'StatusBar'];
  const THEME_ALIAS = { LabelButton: 'Button', ToggleGroup: 'Toggle', Spinner: 'ValueBox', SliderBar: 'Slider' };
  const themeKey = type => THEME_ALIAS[type] || (THEME_KEYS.includes(type) ? type : 'Default');
  const STATES = ['', 'Focused', 'Pressed', 'Disabled'];

  // Propriétés de design raygui (thème et styles nommés)
  const STYLE_FIELDS = [
    ['TextSize', 'number'], ['Radius', 'number'], ['BorderWidth', 'number'], ['TextPadding', 'number'], ['TextSpacing', 'number'],
    ['Align', 'select', ['', 'Left', 'Center', 'Right']],
    ...STATES.flatMap(st => ['Border', 'Base', 'Text'].map(p => [`${p}Color${st}`, 'color'])),
    ['BackgroundColor', 'color'], ['LineColor', 'color'],
  ];
  const THEME_FIELDS = [...STYLE_FIELDS, ['Font', 'text'], ['File', 'text']];

  // Propriétés éditables par type : [clé, type de champ]
  function fields(type) {
    const f = [['Id', 'text'], ['Anchor', 'anchor'], ['Position', 'vec2'], ['Size', 'vec2']];
    if (type === 'Window') f.push(['Title', 'text'], ['Closable', 'bool']);
    else if (type !== 'Image' && type !== 'Container') f.push(['Text', 'text']);
    if (LISTS.includes(type)) f.push(['Items', 'list'], ['Value', 'number']);
    if (type === 'Toggle' || type === 'CheckBox') f.push(['Checked', 'bool']);
    if (NUMBERS.includes(type)) f.push(['Value', 'number'], ['Min', 'number'], ['Max', 'number']);
    if (SLIDERS.includes(type)) f.push(['ShowValue', 'bool']);
    if (type === 'TextBox') f.push(['MaxLength', 'number']);
    if (type === 'Image' || type === 'Button') f.push(['Image', 'text'], ['Tint', 'color']);
    if (ACTIONS.includes(type)) f.push(['Action', 'text']);
    f.push(['Style', 'style'], ['Tooltip', 'text'], ['Visible', 'bool'], ['Enabled', 'bool']);
    return f;
  }

  const create = type => ({ Type: type, Position: [0, 0], ...structuredClone(TYPES[type]), ...(CONTAINERS.includes(type) ? { Children: [] } : {}) });

  const num = v => (v === null || v === undefined || v === '' || isNaN(+v)) ? v : +v;
  const vec = v => Array.isArray(v) ? v.slice(0, 2).map(x => +x || 0) : undefined;

  function fromNode(node, type) {
    const el = { Type: node.Type || type };
    for (const [k, v] of Object.entries(node)) {
      if (k === 'Type') continue;
      if (k === 'Children') el.Children = (v || []).map(c => fromNode(c || {}, ''));
      else if (k === 'Position' || k === 'Size') el[k] = vec(v);
      else if (['Value', 'Min', 'Max', 'MaxLength'].includes(k)) el[k] = num(v);
      else if (['Closable', 'Checked', 'ShowValue', 'Visible', 'Enabled'].includes(k)) el[k] = /^(true|yes|on|1)$/i.test(String(v));
      else el[k] = v;
    }
    if (CONTAINERS.includes(el.Type) && !el.Children) el.Children = [];
    return el;
  }

  function toNode(el, withType = true) {
    const { Type, Children, ...props } = el;
    const node = withType ? { Type } : {};
    for (const [k, v] of Object.entries(props)) {
      if (v === '' || v === undefined || v === null) continue;
      if ((k === 'Visible' || k === 'Enabled' || k === 'Closable') && v === true) continue;
      if ((k === 'Checked' || k === 'ShowValue') && v === false) continue;
      if (k === 'Anchor' && v === 'TopLeft') continue;
      node[k] = v;
    }
    if (Children && Children.length) node.Children = Children.map(c => toNode(c));
    return node;
  }

  function importYaml(text) {
    const doc = Yaml.parse(text) || {};
    if (typeof doc !== 'object' || Array.isArray(doc)) throw new Error('la racine doit être une map');
    const state = { screen: { Size: [800, 600], Scale: 'Fit' }, theme: {}, styles: {}, roots: [] };
    for (const [key, node] of Object.entries(doc)) {
      if (key === 'Screen') state.screen = { Size: vec(node?.Size) || [800, 600], Scale: node?.Scale || 'None' };
      else if (key === 'Styles') state.styles = structuredClone(node || {});
      else if (key === 'Theme') state.theme = structuredClone(node || {});
      else state.roots.push(fromNode(node || {}, key));
    }
    return state;
  }

  function exportYaml(state) {
    const doc = { Screen: { Size: state.screen.Size, Scale: state.screen.Scale } };
    const clean = o => Object.fromEntries(Object.entries(o).filter(([, v]) => v && Object.keys(v).length));
    if (Object.keys(clean(state.theme)).length) doc.Theme = clean(state.theme);
    if (Object.keys(state.styles).length) doc.Styles = state.styles;
    const used = new Set(['Screen', 'Styles', 'Theme']);
    for (const el of state.roots) {
      let key = used.has(el.Type) ? (el.Id || el.Type) : el.Type;
      for (let i = 2; used.has(key); i++) key = `${el.Type}${i}`;
      used.add(key);
      doc[key] = toNode(el, key !== el.Type);
    }
    return Yaml.dump(doc);
  }

  // Taille par défaut et position, reproduisant Screen::layout() de SigmaUi.
  function layout(el, parent, styles) {
    let [w, h] = el.Size || [0, 0];
    if (w <= 0 || h <= 0) {
      const ts = Math.max(10, styles[el.Style]?.TextSize || 0);
      if (CONTAINERS.includes(el.Type)) [w, h] = [parent.w, parent.h];
      else if (el.Type === 'Label') [w, h] = [(el.Text || '').length * ts * 0.62 + 8, ts + 8];
      else if (el.Type === 'CheckBox') [w, h] = [20, 20];
      else [w, h] = [120, 30];
    }
    const a = Math.max(0, ANCHORS.indexOf(el.Anchor || 'TopLeft'));
    const ax = (a % 3) / 2, ay = Math.floor(a / 3) / 2;
    const [px, py] = el.Position || [0, 0];
    return { x: parent.x + parent.w * ax - w * ax + px, y: parent.y + parent.h * ay - h * ay + py, w, h };
  }

  return { ANCHORS, CONTAINERS, SLIDERS, LISTS, TYPES, THEME_KEYS, STYLE_FIELDS, THEME_FIELDS, themeKey, fields, create, importYaml, exportYaml, layout };
})();
