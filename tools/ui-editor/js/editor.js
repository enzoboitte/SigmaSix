(() => {
  const $ = id => document.getElementById(id);
  const h = (tag, props = {}, ...kids) => { const e = Object.assign(document.createElement(tag), props); e.append(...kids); return e; };
  const STORE = 'sigmasix-ui-editor';
  const SAMPLE = `# menu.yml
Screen:
  Size: [800, 600]
  Scale: Fit
Theme:
  Default: {TextSize: 16, BorderWidth: 2, Radius: 4, BorderColor: "#6b8f3a", BaseColor: "#1e2a1e", TextColor: "#d8e8c0", BorderColorFocused: "#a6d65c", BaseColorFocused: "#2b3c2b", TextColorFocused: "#ffffff", BorderColorPressed: "#e0c060", BaseColorPressed: "#3a4a2a", TextColorPressed: "#e0c060", BackgroundColor: "#141c14", LineColor: "#6b8f3a"}
  Button: {Radius: 8}
Styles:
  Header: {TextSize: 28, TextColor: "#e0c060"}
  Footer: {TextSize: 10, TextColor: "#8a9a80"}
Window:
  Title: "SigmaSix"
  Anchor: Center
  Size: [400, 300]
  Children:
    - Type: Label
      Text: "MISSION START"
      Anchor: Top
      Position: [0, 50]
      Style: "Header"
    - Type: Button
      Text: "CAMPAIGN"
      Position: [120, 120]
      Size: [160, 40]
      Action: "StartCampaign"
    - Type: Button
      Text: "SETTINGS"
      Position: [120, 170]
      Size: [160, 40]
      Action: "OpenSettings"
    - Type: Label
      Text: "v1.0.0"
      Anchor: BottomRight
      Position: [-8, -4]
      Style: "Footer"
`;

  let state, sel = null, fileName = 'interface.yml', styleSel = '', themeSel = 'Default';
  let fileHandle = null, savedText = null;   // fichier ouvert (File System Access) et contenu enregistré
  const history = [];
  const rects = new Map();     // chemin JSON -> rectangle à l'écran (référence), '[]' = écran
  const assetUrls = {};        // chemin relatif au .yml -> URL locale (dossier assets choisi)
  const key = p => JSON.stringify(p);
  const imgSrc = path => assetUrls[path] || `../../assets/${$('ymlDir').value}${path}`;
  const findPath = (el, list = state.roots, base = []) => {
    for (let i = 0; i < list.length; i++) {
      if (list[i] === el) return [...base, i];
      const r = list[i].Children && findPath(el, list[i].Children, [...base, i]);
      if (r) return r;
    }
    return null;
  };
  // Conteneur qui reçoit les nouveaux éléments : la sélection si c'est un conteneur, sinon son parent.
  const targetPath = () => { const cur = elAt(sel); return !cur ? [] : cur.Children ? sel : sel.slice(0, -1); };

  // --- Arbre : sélection par chemin d'indices ---
  const listAt = path => path.slice(0, -1).reduce((l, i) => l[i].Children, state.roots);
  const elAt = path => path && listAt(path)[path.at(-1)];
  const snapshot = () => { history.push(JSON.stringify({ state, sel })); if (history.length > 100) history.shift(); };
  const grid = () => Math.max(1, +$('grid').value || 1);
  const snap = v => Math.round(v / grid()) * grid();
  let statusTimer;
  const status = (msg, err) => {
    $('status').textContent = msg; $('status').className = 'status show' + (err ? ' err' : '');
    clearTimeout(statusTimer); statusTimer = setTimeout(() => $('status').classList.remove('show'), err ? 12000 : 5000);
  };

  function load(s) { state = s; sel = null; styleSel = Object.keys(state.styles)[0] || ''; renderAll(); }

  function renderAll() { renderScreen(); renderTree(); renderStage(); renderProps(); renderTheme(); renderStyles(); renderYaml(); renderTarget(); }
  function refresh() { renderTree(); renderStage(); renderYaml(); renderTarget(); }

  function renderTarget() {
    const t = targetPath(), el = elAt(t.length ? t : null);
    $('target').textContent = el ? `${el.Type}${el.Id ? ' #' + el.Id : ''}` : 'Écran (racine)';
  }

  function renderScreen() {
    [$('sw').value, $('sh').value] = state.screen.Size;
    $('scale').value = state.screen.Scale;
  }

  function renderTree() {
    const items = [];
    const walk = (list, path, depth) => list.forEach((el, i) => {
      const p = [...path, i];
      items.push(treeDrop(h('li', { className: JSON.stringify(p) === JSON.stringify(sel) ? 'sel' : '', style: `padding-left:${6 + depth * 14}px`, onclick: () => select(p),
        draggable: true, ondragstart: e => { dragPath = p; e.dataTransfer.setData('text/x-tree', ''); } },
        el.Type, h('small', {}, el.Id || el.Text || el.Title || '')), p));
      if (el.Children) walk(el.Children, p, depth + 1);
    });
    walk(state.roots, [], 0);
    $('tree').replaceChildren(treeDrop(h('li', { className: sel ? 'root' : 'root sel', onclick: () => select(null) }, 'Écran', h('small', {}, `${state.roots.length} élément(s)`)), []), ...items);
  }

  // Glisser dans l'arbre : haut/bas d'une ligne = avant/après, milieu d'un conteneur (ou « Écran ») = dedans.
  let dragPath = null;
  function treeDrop(li, p) {
    const zone = e => {
      const b = li.getBoundingClientRect(), y = (e.clientY - b.top) / b.height;
      if (!p.length || (elAt(p).Children && y > 0.25 && y < 0.75)) return 'in';
      return y < 0.5 ? 'before' : 'after';
    };
    const valid = () => dragPath && key(p.slice(0, dragPath.length)) !== key(dragPath); // ni sur lui-même ni dans ses enfants
    li.ondragover = e => {
      if (!valid()) return;
      e.preventDefault();
      li.dataset.drop = zone(e);
    };
    li.ondragleave = () => delete li.dataset.drop;
    li.ondrop = e => {
      delete li.dataset.drop;
      if (!valid()) return;
      e.preventDefault();
      const z = zone(e), from = dragPath;
      dragPath = null;
      sel = from;
      if (z === 'in') moveTo(p);
      else moveTo(p.slice(0, -1), elAt(p), z === 'after');
    };
    li.ondragend = () => { dragPath = null; };
    return li;
  }

  function renderStage() {
    const [W, H] = state.screen.Size, z = +$('zoom').value, stage = $('stage');
    stage.style.cssText = `width:${W}px;height:${H}px;transform:scale(${z});margin-right:${W * (z - 1)}px;margin-bottom:${H * (z - 1)}px;background-size:${grid()}px ${grid()}px`;
    stage.classList.toggle('grid', grid() >= 5);
    const nodes = [];
    rects.clear();
    rects.set('[]', { x: 0, y: 0, w: W, h: H });
    const draw = (list, parent, path, inherited) => list.forEach((el, i) => {
      const p = [...path, i], r = Model.layout(el, parent, state.styles);
      rects.set(key(p), r);
      const own = state.styles[el.Style] || {};
      const st = { ...state.theme.Default, ...(state.theme[Model.themeKey(el.Type)] || {}), ...inherited, ...own };
      nodes.push(renderEl(el, r, st, p));
      if (el.Children) draw(el.Children, r, p, Model.CONTAINERS.includes(el.Type) ? { ...inherited, ...own } : inherited);
    });
    draw(state.roots, { x: 0, y: 0, w: W, h: H }, [], {});
    stage.replaceChildren(...nodes);
  }

  function renderEl(el, r, st, path) {
    const t = el.Type, text = el.Text || '';
    const on = (t === 'Toggle' || t === 'CheckBox') && !!(el.Checked || +el.Value);
    const ia = !['Window', 'Panel', 'GroupBox', 'Container', 'Line', 'Label', 'StatusBar', 'Image', 'ProgressBar'].includes(t);
    const div = h('div', { className: `el el-${t}${on ? ' on' : ''}${ia ? ' ia' : ''}${el.Enabled === false ? ' disabled' : ''}${el.Visible === false ? ' hidden' : ''}` });
    let css = `left:${r.x}px;top:${r.y}px;width:${r.w}px;height:${r.h}px;font-size:${st.TextSize || 10}px;`;
    [['', 'n'], ['Focused', 'f'], ['Pressed', 'p'], ['Disabled', 'd']].forEach(([state, v]) =>
      ['Border', 'Base', 'Text'].forEach(p => { const c = st[`${p}Color${state}`]; if (c) css += `--${v}-${p.toLowerCase()}:${c};`; }));
    if (st.BackgroundColor) css += `--rg-bg:${st.BackgroundColor};`;
    if (st.LineColor) css += `--rg-line:${st.LineColor};`;
    if (st.BorderWidth !== undefined) css += `--bw:${st.BorderWidth}px;`;
    if (st.Radius !== undefined) css += `--rad:${st.Radius}px;`;
    if (st.TextPadding !== undefined) css += `--pad:${st.TextPadding}px;`;
    if (st.TextSpacing !== undefined) css += `letter-spacing:${st.TextSpacing}px;`;
    if (st.Align) css += `justify-content:${{ Left: 'flex-start', Center: 'center', Right: 'flex-end' }[st.Align]};`;
    div.style.cssText = css;
    div.dataset.path = key(path);
    if (t === 'Window' || t === 'Panel' || t === 'GroupBox') {
      if (t !== 'Panel' || text) div.append(h('div', { className: 'title' }, h('span', {}, el.Title || text), h('span', {}, t === 'Window' && el.Closable !== false ? '✕' : '')));
    } else if (t === 'ToggleGroup') {
      div.append(...(el.Items || []).map((it, i) => h('div', { className: `el-tg${i === (+el.Value || 0) ? ' on' : ''}` }, it)));
    } else if (t === 'ComboBox' || t === 'Dropdown') {
      div.append((el.Items || [])[+el.Value || 0] || '');
    } else if (t === 'CheckBox') {
      div.append(h('span', {}, text));
    } else if (Model.SLIDERS.includes(t)) {
      const min = +el.Min || 0, max = +el.Max || 100, pct = Math.max(0, Math.min(1, ((+el.Value || 0) - min) / ((max - min) || 1)));
      div.append(h('div', { className: 'fill', style: `width:${pct * 100}%` }));
      if (text) div.append(h('span', { className: 'side l' }, text));
      if (el.ShowValue) div.append(h('span', { className: 'side r' }, String(Math.round(+el.Value || 0))));
    } else if (t === 'ValueBox' || t === 'Spinner') {
      div.append(String(el.Value ?? 0));
      if (text) div.append(h('span', { className: 'side l' }, text));
    } else if (t === 'Image') {
      if (!el.Image) div.append('image'), div.classList.add('missing');
    } else if (t !== 'Line' && t !== 'Container') {
      div.append(text);
    }
    if (el.Image && (t === 'Image' || t === 'Button')) {
      const img = h('img', { src: imgSrc(el.Image), draggable: false, onerror: () => {
        img.remove();
        if (t === 'Image') div.classList.add('missing'), div.append(el.Image.split('/').pop());
      } });
      if (el.Tint) img.style.filter = `drop-shadow(0 0 0 ${el.Tint})`;
      div.prepend(img);
      if (t === 'Button') div.style.background = 'none', div.style.border = 'none';
    }
    div.onpointerdown = e => { e.stopPropagation(); select(path); startDrag(e, el, 'Position', r); };
    if (JSON.stringify(path) === JSON.stringify(sel)) {
      div.classList.add('sel');
      const handle = h('div', { className: 'handle' });
      handle.onpointerdown = e => { e.stopPropagation(); startDrag(e, el, 'Size', r); };
      div.append(handle);
    }
    return div;
  }

  function startDrag(e, el, key, r) {
    snapshot();
    const z = +$('zoom').value, start = key === 'Size' ? [r.w, r.h] : [...(el.Position || [0, 0])], [mx, my] = [e.clientX, e.clientY];
    let moved = false;
    const move = ev => {
      const dx = (ev.clientX - mx) / z, dy = (ev.clientY - my) / z;
      if (!moved && Math.abs(dx) + Math.abs(dy) < 2) return;
      moved = true;
      if (key === 'Size') {
        const w = Math.max(4, snap(start[0] + dx)); // Maj (ou image) : garde les proportions
        const keepRatio = ev.shiftKey !== (el.Type === 'Image');
        el.Size = [w, keepRatio ? Math.max(4, Math.round(w * start[1] / start[0])) : Math.max(4, snap(start[1] + dy))];
      } else el.Position = [snap(start[0] + dx), snap(start[1] + dy)];
      renderStage();
    };
    const up = () => {
      removeEventListener('pointermove', move); removeEventListener('pointerup', up);
      if (moved) { refresh(); renderProps(); } else history.pop();
    };
    addEventListener('pointermove', move); addEventListener('pointerup', up);
  }

  function select(path) {
    if (JSON.stringify(path) === JSON.stringify(sel)) return;
    sel = path; renderTree(); renderStage(); renderProps(); renderTarget();
  }

  // --- Formulaires ---
  function field(obj, key, kind, onChange, options = []) {
    const set = v => { if (v === '' || v === undefined || (Array.isArray(v) && !v.length)) delete obj[key]; else obj[key] = v; onChange(); };
    const val = obj[key];
    let input;
    if (kind === 'bool') {
      const def = key === 'Visible' || key === 'Enabled' || key === 'Closable';
      input = h('input', { type: 'checkbox', checked: val ?? def, onchange: e => set(e.target.checked === def ? undefined : e.target.checked) });
    } else if (kind === 'vec2') {
      const v = val || [0, 0];
      const n = i => h('input', { type: 'number', value: v[i], oninput: e => { const c = [...(obj[key] || [0, 0])]; c[i] = +e.target.value || 0; set(c); } });
      input = h('div', { className: 'row' }, n(0), n(1));
    } else if (kind === 'anchor') {
      // Grille 3×3 : le point d'ancrage dans le parent (et pivot de l'élément).
      input = h('div', { className: 'anchor', tabIndex: 0 }, ...Model.ANCHORS.map(a => h('button', {
        type: 'button', title: a, className: a === (val || 'TopLeft') ? 'on' : '',
        onclick: () => { snapshot(); set(a === 'TopLeft' ? undefined : a); renderProps(); } })));
    } else if (kind === 'select' || kind === 'style') {
      const opts = kind === 'style' ? ['', ...Object.keys(state.styles)] : options;
      input = h('select', { onchange: e => set(e.target.value) }, ...opts.map(o => h('option', { value: o, selected: o === (val ?? '') }, o || '—')));
    } else if (kind === 'color') {
      const text = h('input', { type: 'text', value: val || '', placeholder: '#rrggbb', oninput: e => { pick.value = e.target.value.slice(0, 7); set(e.target.value); } });
      const pick = h('input', { type: 'color', value: (val || '#ffffff').slice(0, 7), oninput: e => { text.value = e.target.value; set(e.target.value); } });
      input = h('div', { className: 'row' }, text, pick);
    } else if (kind === 'list') {
      input = h('input', { type: 'text', value: (val || []).join(';'), title: 'Séparateur ;', oninput: e => set(e.target.value.split(';').map(s => s.trim()).filter(Boolean)) });
    } else {
      input = h('input', { type: kind === 'number' ? 'number' : 'text', value: val ?? '', oninput: e => set(kind === 'number' && e.target.value !== '' ? +e.target.value : e.target.value) });
      if (kind === 'image') input.setAttribute('list', 'assetList');
    }
    input.addEventListener('focusin', snapshot, { once: true });
    const LABELS = { Anchor: 'Ancre', Position: 'Position', Size: 'Taille', Title: 'Titre', Text: 'Texte', Items: 'Choix', Tint: 'Teinte',
      Value: 'Valeur', Checked: 'Coché', ShowValue: 'Afficher valeur', MaxLength: 'Longueur max', Visible: 'Visible', Enabled: 'Actif', Closable: 'Fermable' };
    return [h('label', { title: key }, LABELS[key] || key), input];
  }

  // Liste des conteneurs où l'élément sélectionné peut être déplacé (pas lui-même ni ses descendants).
  function parentField(el) {
    const opts = [['[]', 'Écran (racine)']];
    const walk = (list, path, depth) => list.forEach((c, i) => {
      const p = [...path, i];
      if (c === el) return;
      if (c.Children) { opts.push([key(p), `${'  '.repeat(depth + 1)}${c.Type}${c.Id ? ' #' + c.Id : ''}`]); walk(c.Children, p, depth + 1); }
    });
    walk(state.roots, [], 0);
    const cur = key(sel.slice(0, -1));
    return [h('label', {}, 'Dans'), h('select', { onchange: e => moveTo(JSON.parse(e.target.value)) },
      ...opts.map(([v, t]) => h('option', { value: v, selected: v === cur }, t)))];
  }

  const PROP_GROUPS = [
    ['Disposition', ['Anchor', 'Position', 'Size']],
    ['Contenu', ['Title', 'Text', 'Items', 'Image', 'Tint', 'Value', 'Min', 'Max', 'Checked', 'ShowValue', 'MaxLength']],
    ['Apparence', ['Style']],
    ['Code & comportement', ['Id', 'Action', 'Tooltip', 'Visible', 'Enabled', 'Closable']],
  ];

  function renderProps() {
    const el = elAt(sel);
    $('screenProps').hidden = !!el;
    if (!el) return $('props').replaceChildren();
    const tk = Model.themeKey(el.Type), fields = Model.fields(el.Type);
    $('props').replaceChildren(
      h('div', { className: 'type-head' }, h('strong', {}, el.Type),
        h('button', { type: 'button', className: 'small', title: 'Modifier le design de tous les ' + tk, onclick: () => { themeSel = tk; renderTheme(); showTab('theme'); } }, `Thème ${tk} →`)),
      ...PROP_GROUPS.flatMap(([title, keys]) => {
        const rows = fields.filter(([k]) => keys.includes(k)).flatMap(([k, kind]) => field(el, k, kind, refresh));
        if (title === 'Disposition') rows.unshift(...parentField(el));
        return rows.length ? [h('h4', {}, title), h('div', { className: 'props' }, ...rows)] : [];
      }));
  }

  // Formulaire de design (thème ou style nommé). inherit : valeurs effectives sans surcharge, affichées en grisé.
  function designForm(obj, inherit, withFiles, changed) {
    const done = () => { renderStage(); renderYaml(); changed(); };
    const reset = k => h('button', { type: 'button', className: 'clr', title: 'Revenir à la valeur héritée', onclick: () => { snapshot(); delete obj[k]; done(); } }, '×');
    const num = k => h('div', { className: 'cell' + (k in obj ? ' set' : '') },
      h('input', { type: 'number', value: obj[k] ?? '', placeholder: inherit[k] ?? '', onfocus: snapshot,
        oninput: e => { e.target.value === '' ? delete obj[k] : (obj[k] = +e.target.value); e.target.parentElement.classList.toggle('set', k in obj); renderStage(); renderYaml(); } }),
      k in obj ? reset(k) : '');
    const color = k => {
      const v = obj[k] ?? inherit[k];
      return h('div', { className: 'cell swatch' + (k in obj ? ' set' : '') + (v ? '' : ' none'), title: `${k} : ${v || 'non défini'}` },
        h('input', { type: 'color', value: (v || '#000000').slice(0, 7), onclick: snapshot,
          oninput: e => { obj[k] = e.target.value + (obj[k] || '').slice(7); e.target.parentElement.classList.add('set', 'live'); renderStage(); renderYaml(); },
          onchange: done }),
        k in obj ? reset(k) : '');
    };
    const row = (label, ...cells) => [h('label', {}, label), h('div', { className: 'row' }, ...cells)];
    const align = h('select', { onfocus: snapshot, onchange: e => { e.target.value ? (obj.Align = e.target.value) : delete obj.Align; done(); } },
      ...['', 'Left', 'Center', 'Right'].map(o => h('option', { value: o, selected: o === (obj.Align || '') }, o || `hérité (${inherit.Align || '—'})`)));
    const text = k => h('input', { type: 'text', value: obj[k] || '', placeholder: 'fonts/terminal.ttf', onfocus: snapshot, onchange: e => { e.target.value ? (obj[k] = e.target.value) : delete obj[k]; done(); } });

    const STATES = [['', 'Normal'], ['Focused', 'Survol'], ['Pressed', 'Pressé'], ['Disabled', 'Désactivé']];
    const matrix = h('div', { className: 'matrix' }, h('span'), ...STATES.map(([, l]) => h('span', { className: 'head' }, l)),
      ...[['Base', 'Fond'], ['Border', 'Bordure'], ['Text', 'Texte']].flatMap(([p, l]) => [h('span', { className: 'head' }, l), ...STATES.map(([s]) => color(`${p}Color${s}`))]));

    return [
      h('h4', {}, 'Couleurs'), matrix,
      h('h4', {}, 'Forme'), h('div', { className: 'props' }, ...row('Bordure (px)', num('BorderWidth')), ...row('Arrondi (px)', num('Radius'))),
      h('h4', {}, 'Texte'), h('div', { className: 'props' }, ...row('Taille', num('TextSize')), ...row('Alignement', align),
        ...row('Marge', num('TextPadding')), ...row('Espacement', num('TextSpacing')), ...(withFiles ? row('Police', text('Font')) : [])),
      h('h4', {}, 'Fenêtres & lignes'), h('div', { className: 'props' }, ...row('Fond panneau', color('BackgroundColor')), ...row('Lignes', color('LineColor'))),
      ...(withFiles ? [h('h4', {}, 'Fichier raygui'), h('div', { className: 'props' }, ...row('Style .rgs', text('File')))] : []),
    ];
  }

  function renderTheme() {
    $('themeChips').replaceChildren(...Model.THEME_KEYS.map(k => h('button', {
      className: (k === themeSel ? 'on' : '') + (Object.keys(state.theme[k] || {}).length ? ' set' : ''),
      onclick: () => { themeSel = k; renderTheme(); } }, k)));
    const inherit = themeSel === 'Default' ? Model.RAYGUI : { ...Model.RAYGUI, ...state.theme.Default };
    $('themeProps').replaceChildren(...designForm(state.theme[themeSel] ??= {}, inherit, themeSel === 'Default', renderTheme));
  }

  function renderStyles() {
    $('styleList').replaceChildren(...Object.keys(state.styles).map(n => h('option', { value: n, selected: n === styleSel }, n)));
    const st = state.styles[styleSel];
    $('styleProps').replaceChildren(...(st ? designForm(st, { ...Model.RAYGUI, ...state.theme.Default }, false, renderStyles) : [h('p', { className: 'empty' }, 'Aucun style. Clique sur + pour en créer un.')]));
  }

  function showTab(name, group = 'right') {
    document.querySelectorAll(`.tabs[data-group=${group}] button`).forEach(b => b.classList.toggle('on', b.dataset.tab === name));
    document.querySelectorAll(`.tab[data-group=${group}]`).forEach(t => t.classList.toggle('on', t.id === `tab-${name}`));
  }

  function renderYaml() {
    const text = Model.exportYaml(state);
    if (document.activeElement !== $('yaml')) $('yaml').value = text;
    $('fileName').textContent = fileName + (text !== savedText ? ' •' : '');
    $('fileName').title = fileHandle ? 'Ctrl+S réécrit ce fichier' : 'Pas encore lié à un fichier : Ctrl+S demandera où enregistrer';
    try { localStorage.setItem(STORE, text); } catch {}
  }

  // --- Commandes ---
  // Ajoute un élément dans le conteneur `parent` (chemin, [] = écran) ; `at` = point en coordonnées écran.
  function add(type, parent = targetPath(), at = null, props = {}) {
    snapshot();
    const el = Object.assign(Model.create(type), props), list = parent.length ? elAt(parent).Children : state.roots;
    if (at) {
      const r = rects.get(key(parent));
      el.Position = [snap(at.x - r.x), snap(at.y - r.y)];
    } else if (type === 'Window' && !parent.length) el.Anchor = 'Center';
    else el.Position = [grid() * 2, grid() * 2 + (parent.length ? 24 : 0) + (list.length % 10) * grid() * 3];
    list.push(el);
    sel = [...parent, list.length - 1];
    refresh(); renderProps(); showTab('props');
  }

  // Déplace la sélection dans un autre conteneur en gardant sa position à l'écran.
  // ref : élément frère avant/après lequel insérer (sinon à la fin).
  function moveTo(parent, ref = null, after = false) {
    const el = elAt(sel), dest = parent.length ? elAt(parent) : null;
    if (ref === el) return;
    snapshot();
    const a = Math.max(0, Model.ANCHORS.indexOf(el.Anchor || 'TopLeft')), ax = (a % 3) / 2, ay = Math.floor(a / 3) / 2;
    const from = rects.get(key(sel.slice(0, -1))), to = rects.get(key(parent));
    const [px, py] = el.Position || [0, 0];
    el.Position = [px + from.x + from.w * ax - to.x - to.w * ax, py + from.y + from.h * ay - to.y - to.h * ay].map(Math.round);
    listAt(sel).splice(sel.at(-1), 1);
    const list = dest ? dest.Children : state.roots, i = ref ? list.indexOf(ref) : -1;
    list.splice(i < 0 ? list.length : i + (after ? 1 : 0), 0, el);
    sel = findPath(el);
    refresh(); renderProps();
  }

  // Assets : dossier choisi par l'utilisateur, chemins réécrits relativement au .yml.
  function relToYml(path) {
    const dir = $('ymlDir').value.split('/').filter(Boolean), parts = path.split('/');
    let i = 0;
    while (i < dir.length && parts[i] === dir[i]) i++;
    return [...dir.slice(i).map(() => '..'), ...parts.slice(i)].join('/');
  }

  function renderAssets() {
    const q = $('assetFilter').value.toLowerCase(), paths = Object.keys(assetUrls);
    $('assetList').replaceChildren(...paths.map(p => h('option', { value: p })));
    if (!paths.length) return;
    $('assets').replaceChildren(...paths.filter(p => p.toLowerCase().includes(q)).slice(0, 300).map(p => {
      const img = h('img', { src: assetUrls[p], loading: 'lazy', alt: '' });
      const card = h('div', { className: 'asset', title: `${p}\nClic : ajouter / remplacer l'image sélectionnée — ou glisser sur l'écran`, draggable: true },
        img, h('span', {}, p.split('/').pop()));
      const size = () => img.naturalWidth ? { Size: [img.naturalWidth, img.naturalHeight] } : {};
      card.onclick = () => {
        const cur = elAt(sel);
        if (cur && cur.Type === 'Image') { snapshot(); Object.assign(cur, { Image: p }, size()); refresh(); renderProps(); }
        else add('Image', targetPath(), null, { Image: p, ...size() });
      };
      card.ondragstart = e => e.dataTransfer.setData('text/x-asset', JSON.stringify({ Image: p, ...size() }));
      return card;
    }));
  }

  let assetFiles = [];
  function indexAssets() {
    Object.values(assetUrls).forEach(URL.revokeObjectURL);
    for (const k in assetUrls) delete assetUrls[k];
    for (const f of assetFiles) {
      const path = f.webkitRelativePath.split('/').slice(1).join('/'); // sans le dossier choisi lui-même
      assetUrls[relToYml(path)] = URL.createObjectURL(f);
    }
    renderAssets(); renderStage();
  }
  $('assetDir').onchange = e => {
    assetFiles = [...e.target.files].filter(f => /\.(png|jpe?g|gif|bmp|webp)$/i.test(f.name));
    indexAssets();
    status(`${assetFiles.length} image(s) chargée(s).`);
  };
  $('ymlDir').addEventListener('change', indexAssets);
  $('assetFilter').oninput = renderAssets;

  // Dépôt sur l'écran : dans le conteneur survolé, sinon à la racine.
  $('stageWrap').ondragover = e => e.preventDefault();
  $('stageWrap').ondrop = e => {
    e.preventDefault();
    const type = e.dataTransfer.getData('text/x-type'), asset = e.dataTransfer.getData('text/x-asset');
    if (!type && !asset) return;
    const z = +$('zoom').value, box = $('stage').getBoundingClientRect();
    const at = { x: (e.clientX - box.left) / z, y: (e.clientY - box.top) / z };
    // Les éléments sont à plat dans le DOM : conteneur le plus profond (puis le plus haut dessiné) sous le point.
    let parent = [];
    for (const [k, r] of rects) {
      const p = JSON.parse(k);
      if (p.length && p.length >= parent.length && elAt(p).Children && at.x >= r.x && at.y >= r.y && at.x <= r.x + r.w && at.y <= r.y + r.h) parent = p;
    }
    add(type || 'Image', parent, at, asset ? JSON.parse(asset) : {});
  };

  const commands = {
    new: () => { if (confirm('Créer une nouvelle interface vide ?')) { snapshot(); fileName = 'interface.yml'; fileHandle = null; savedText = null; load({ screen: { Size: [800, 600], Scale: 'Fit' }, theme: {}, styles: {}, roots: [] }); } },
    open: async () => {
      if (!window.showOpenFilePicker) return $('open').click();
      let handle;
      try { [handle] = await showOpenFilePicker({ types: YML }); } catch { return; }
      const file = await handle.getFile();
      openText(file.name, await file.text(), handle);
    },
    importStyles: () => { importStyles = true; $('open').click(); },
    save: () => save(false),
    saveAs: () => save(true),
    styles: () => writeFile('styles.yml', Model.exportYaml({ ...state, roots: [] }).replace(/^Screen:\n(  .*\n)*/, '')),
    zoomIn: () => setZoom(+$('zoom').value + 0.1),
    zoomOut: () => setZoom(+$('zoom').value - 0.1),
    zoomFit: () => { const w = $('stageWrap'); setZoom(Math.min((w.clientWidth - 64) / state.screen.Size[0], (w.clientHeight - 64) / state.screen.Size[1])); },
    toRoot: () => select(null),
    undo: () => { const s = history.pop(); if (!s) return; ({ state, sel } = JSON.parse(s)); renderAll(); },
    delete: () => { if (!sel) return; snapshot(); listAt(sel).splice(sel.at(-1), 1); sel = null; refresh(); renderProps(); },
    duplicate: () => {
      const el = elAt(sel); if (!el) return;
      snapshot();
      const copy = structuredClone(el), list = listAt(sel);
      copy.Position = [(copy.Position?.[0] || 0) + grid(), (copy.Position?.[1] || 0) + grid()];
      if (copy.Id) copy.Id += '_copy';
      list.splice(sel.at(-1) + 1, 0, copy);
      sel = [...sel.slice(0, -1), sel.at(-1) + 1];
      refresh(); renderProps();
    },
    addStyle: () => {
      const name = prompt('Nom du style :');
      if (!name || state.styles[name]) return;
      snapshot(); state.styles[name] = { TextSize: 10 }; styleSel = name; renderStyles(); renderProps(); renderYaml();
    },
    delStyle: () => { if (!styleSel) return; snapshot(); delete state.styles[styleSel]; styleSel = Object.keys(state.styles)[0] || ''; renderStyles(); renderProps(); refresh(); },
    applyYaml: () => { try { snapshot(); load(Model.importYaml($('yaml').value)); status('YAML appliqué.'); } catch (e) { history.pop(); status(e.message, true); } },
  };

  const YML = [{ description: 'YAML', accept: { 'text/yaml': ['.yml', '.yaml'] } }];

  function setZoom(z) {
    $('zoom').value = Math.max(0.25, Math.min(2, Math.round(z * 20) / 20));
    $('zoomLabel').textContent = Math.round($('zoom').value * 100) + '%';
    renderStage();
  }

  // Écrit dans `handle` si fourni ; sinon demande où enregistrer (Chrome/Edge) ou télécharge (autres navigateurs).
  async function writeFile(name, text, handle = null) {
    if (!handle && window.showSaveFilePicker) {
      try { handle = await showSaveFilePicker({ suggestedName: name, types: YML }); } catch { return null; }
    }
    if (!handle) {
      const a = h('a', { href: URL.createObjectURL(new Blob([text], { type: 'text/yaml' })), download: name });
      a.click(); URL.revokeObjectURL(a.href);
      status(`${name} téléchargé : ce navigateur (Firefox, Safari) ne peut pas réécrire un fichier. Pour choisir où l'enregistrer et remplacer l'ancien, active « Toujours demander où enregistrer les fichiers » dans ses paramètres, ou ouvre l'éditeur dans Chrome/Edge.`);
      return null;
    }
    try {
      const w = await handle.createWritable();
      await w.write(text); await w.close();
      status(`${handle.name} enregistré.`);
      return handle;
    } catch (err) { status(`Échec de l'enregistrement : ${err.message}`, true); return null; }
  }

  async function save(as) {
    const text = Model.exportYaml(state);
    const handle = await writeFile(fileName, text, as ? null : fileHandle);
    if (handle) { fileHandle = handle; fileName = handle.name; savedText = text; renderYaml(); }
  }

  // stylesOnly : n'importe que Theme/Styles (bouton « Importer un styles.yml »), même si le fichier a des éléments.
  function openText(name, text, handle = null, stylesOnly = false) {
    try {
      if (text.startsWith('SSYE')) throw new Error(`${name} est chiffré (copie de build/assets) : ouvre celui de assets/ dans les sources.`);
      snapshot();
      let doc;
      try { doc = Model.importYaml(text); } catch (err) { throw new Error(`${name} : YAML invalide — ${err.message}`); }
      const hasStyles = Object.keys(doc.theme).length || Object.keys(doc.styles).length;
      if (stylesOnly && !hasStyles) throw new Error(`${name} ne contient ni « Theme: » ni « Styles: ».`);
      if (stylesOnly || (!doc.roots.length && hasStyles)) {
        // Fichier de styles seul : fusionné dans l'interface en cours (et Ctrl+S ne l'écrasera pas).
        for (const [k, v] of Object.entries(doc.theme)) state.theme[k] = { ...state.theme[k], ...v };
        Object.assign(state.styles, doc.styles);
        styleSel = styleSel || Object.keys(state.styles)[0] || '';
        renderAll(); showTab('theme');
        status(`${name} : thème et ${Object.keys(doc.styles).length} style(s) importés.`);
      } else {
        fileHandle = handle; fileName = name;
        load(doc);
        savedText = Model.exportYaml(state); renderYaml();
        status(`${name} ouvert.` + (handle ? ' Ctrl+S le réécrira.' : ''));
      }
    } catch (err) { history.pop(); status(err.message, true); }
  }

  let importStyles = false;
  async function openFile(file, stylesOnly) {
    try { openText(file.name, await file.text(), null, stylesOnly); }
    catch (err) { status(`Lecture de ${file.name} impossible : ${err.message}`, true); }
  }

  // --- Événements ---
  $('palette').append(...Object.entries(Model.PALETTE).flatMap(([group, types]) => [h('h4', {}, group), h('div', { className: 'palette' },
    ...types.map(t => h('button', { draggable: true, onclick: () => add(t), ondragstart: e => e.dataTransfer.setData('text/x-type', t) }, t)))]));
  document.querySelectorAll('.tabs button').forEach(b => b.onclick = () => showTab(b.dataset.tab, b.parentElement.dataset.group));
  document.querySelectorAll('[data-cmd]').forEach(b => b.addEventListener('click', e => { e.preventDefault(); commands[b.dataset.cmd](); }));
  $('stageWrap').onpointerdown = () => select(null);
  $('styleList').onchange = e => { styleSel = e.target.value; renderStyles(); };
  ['grid', 'ymlDir'].forEach(id => $(id).addEventListener('input', renderStage));
  $('zoom').addEventListener('input', () => setZoom(+$('zoom').value));
  ['sw', 'sh'].forEach((id, i) => $(id).addEventListener('change', e => { snapshot(); state.screen.Size[i] = Math.max(1, +e.target.value || 1); renderStage(); renderYaml(); }));
  $('scale').onchange = e => { snapshot(); state.screen.Scale = e.target.value; renderYaml(); };
  $('yaml').addEventListener('blur', renderYaml);
  // Glisser un .yml n'importe où (hors écran) : ouvre le fichier ; un fichier sans éléments n'importe que les styles.
  addEventListener('dragover', e => { if (e.dataTransfer.types.includes('Files')) e.preventDefault(); });
  addEventListener('drop', e => {
    const file = e.dataTransfer.files[0];
    if (!file) return;
    e.preventDefault();
    if (!/\.ya?ml$/i.test(file.name)) return status(`${file.name} n'est pas un .yml`, true);
    openFile(file, e.target.closest?.('#tab-theme, #tab-styles') != null);
  });
  $('open').onchange = async e => {
    const file = e.target.files[0]; if (!file) return;
    await openFile(file, importStyles);
    importStyles = false;
    e.target.value = '';
  };
  addEventListener('keydown', e => {
    const ctrl = e.ctrlKey || e.metaKey, k = e.key.toLowerCase();
    if (ctrl && (k === 's' || k === 'o')) { e.preventDefault(); return k === 'o' ? commands.open() : save(e.shiftKey); }
    if (/INPUT|TEXTAREA|SELECT/.test(document.activeElement?.tagName)) return;
    const el = elAt(sel);
    if (ctrl && e.key === 'z') commands.undo();
    else if (ctrl && e.key === 'd') commands.duplicate();
    else if (e.key === 'Delete' || e.key === 'Backspace') commands.delete();
    else if (el && e.key.startsWith('Arrow')) {
      const step = e.shiftKey ? grid() : 1, p = [...(el.Position || [0, 0])];
      p[0] += { ArrowLeft: -step, ArrowRight: step }[e.key] || 0;
      p[1] += { ArrowUp: -step, ArrowDown: step }[e.key] || 0;
      snapshot(); el.Position = p; refresh(); renderProps();
    } else return;
    e.preventDefault();
  });

  let saved = null;
  try { saved = localStorage.getItem(STORE); } catch {}
  try { load(Model.importYaml(saved || SAMPLE)); } catch { load(Model.importYaml(SAMPLE)); }
})();
