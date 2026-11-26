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
  const history = [];

  // --- Arbre : sélection par chemin d'indices ---
  const listAt = path => path.slice(0, -1).reduce((l, i) => l[i].Children, state.roots);
  const elAt = path => path && listAt(path)[path.at(-1)];
  const snapshot = () => { history.push(JSON.stringify({ state, sel })); if (history.length > 100) history.shift(); };
  const grid = () => Math.max(1, +$('grid').value || 1);
  const snap = v => Math.round(v / grid()) * grid();
  const status = (msg, err) => { $('status').textContent = msg; $('status').className = 'status' + (err ? ' err' : ''); };

  function load(s) { state = s; sel = null; styleSel = Object.keys(state.styles)[0] || ''; renderAll(); }

  function renderAll() { renderScreen(); renderTree(); renderStage(); renderProps(); renderTheme(); renderStyles(); renderYaml(); }
  function refresh() { renderTree(); renderStage(); renderYaml(); }

  function renderScreen() {
    [$('sw').value, $('sh').value] = state.screen.Size;
    $('scale').value = state.screen.Scale;
  }

  function renderTree() {
    const items = [];
    const walk = (list, path, depth) => list.forEach((el, i) => {
      const p = [...path, i];
      items.push(h('li', { className: JSON.stringify(p) === JSON.stringify(sel) ? 'sel' : '', style: `padding-left:${6 + depth * 14}px`, onclick: () => select(p) },
        el.Type, h('small', {}, el.Id || el.Text || el.Title || '')));
      if (el.Children) walk(el.Children, p, depth + 1);
    });
    walk(state.roots, [], 0);
    $('tree').replaceChildren(...items);
  }

  function renderStage() {
    const [W, H] = state.screen.Size, z = +$('zoom').value, stage = $('stage');
    stage.style.cssText = `width:${W}px;height:${H}px;transform:scale(${z});margin-right:${W * (z - 1)}px;margin-bottom:${H * (z - 1)}px;background-size:${grid()}px ${grid()}px`;
    stage.classList.toggle('grid', grid() >= 5);
    const nodes = [];
    const draw = (list, parent, path, inherited) => list.forEach((el, i) => {
      const p = [...path, i], r = Model.layout(el, parent, state.styles);
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
      div.append(el.Image || 'image');
    } else if (t !== 'Line' && t !== 'Container') {
      div.append(text);
    }
    if (el.Image && (t === 'Image' || t === 'Button')) {
      const img = h('img', { src: $('base').value + el.Image, onerror: () => img.remove() });
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
      el[key] = key === 'Size' ? [Math.max(4, snap(start[0] + dx)), Math.max(4, snap(start[1] + dy))] : [snap(start[0] + dx), snap(start[1] + dy)];
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
    sel = path; renderTree(); renderStage(); renderProps();
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
    } else if (kind === 'select' || kind === 'anchor' || kind === 'style') {
      const opts = kind === 'anchor' ? Model.ANCHORS : kind === 'style' ? ['', ...Object.keys(state.styles)] : options;
      input = h('select', { onchange: e => set(kind === 'anchor' && e.target.value === 'TopLeft' ? undefined : e.target.value) },
        ...opts.map(o => h('option', { value: o, selected: o === (val ?? (kind === 'anchor' ? 'TopLeft' : '')) }, o || '—')));
    } else if (kind === 'color') {
      const text = h('input', { type: 'text', value: val || '', placeholder: '#rrggbb', oninput: e => { pick.value = e.target.value.slice(0, 7); set(e.target.value); } });
      const pick = h('input', { type: 'color', value: (val || '#000000').slice(0, 7), oninput: e => { text.value = e.target.value; set(e.target.value); } });
      input = h('div', { className: 'row' }, text, pick);
    } else if (kind === 'list') {
      input = h('input', { type: 'text', value: (val || []).join(';'), title: 'Séparateur ;', oninput: e => set(e.target.value.split(';').map(s => s.trim()).filter(Boolean)) });
    } else {
      input = h('input', { type: kind === 'number' ? 'number' : 'text', value: val ?? '', oninput: e => set(kind === 'number' && e.target.value !== '' ? +e.target.value : e.target.value) });
    }
    input.addEventListener('focusin', snapshot, { once: true });
    return [h('label', {}, key), input];
  }

  function renderProps() {
    const el = elAt(sel);
    $('props').replaceChildren(...(el
      ? [h('label', {}, 'Type'), h('strong', {}, el.Type), ...Model.fields(el.Type).flatMap(([k, kind]) => field(el, k, kind, refresh))]
      : [h('p', { className: 'empty' }, 'Sélectionne un élément ou ajoute-en un depuis la palette.')]));
  }

  const styleForm = (obj, list) => obj ? list.flatMap(([k, kind, opts]) => field(obj, k, kind, () => { renderStage(); renderYaml(); }, opts)) : [];

  function renderTheme() {
    $('themeList').replaceChildren(...Model.THEME_KEYS.map(k => h('option', { value: k, selected: k === themeSel }, k + (Object.keys(state.theme[k] || {}).length ? ' •' : ''))));
    $('themeProps').replaceChildren(...styleForm(state.theme[themeSel] ??= {}, Model.THEME_FIELDS));
  }

  function renderStyles() {
    $('styleList').replaceChildren(...Object.keys(state.styles).map(n => h('option', { value: n, selected: n === styleSel }, n)));
    const st = state.styles[styleSel];
    $('styleProps').replaceChildren(...(st ? styleForm(st, Model.STYLE_FIELDS) : [h('p', { className: 'empty' }, 'Aucun style.')]));
  }

  function renderYaml() {
    const text = Model.exportYaml(state);
    if (document.activeElement !== $('yaml')) $('yaml').value = text;
    try { localStorage.setItem(STORE, text); } catch {}
  }

  // --- Commandes ---
  function add(type) {
    snapshot();
    const el = Model.create(type), cur = elAt(sel);
    let list = state.roots, base = [];
    if (cur && cur.Children) { list = cur.Children; base = sel; }
    else if (cur) { list = listAt(sel); base = sel.slice(0, -1); }
    if (list !== state.roots || type !== 'Window') el.Position = [grid() * 2, grid() * 2 + (list === state.roots ? 0 : 24) + (list.length % 10) * grid() * 3];
    if (type === 'Window' && list === state.roots) el.Anchor = 'Center';
    list.push(el);
    sel = [...base, list.length - 1];
    refresh(); renderProps();
  }

  const commands = {
    new: () => { if (confirm('Créer une nouvelle interface vide ?')) { snapshot(); fileName = 'interface.yml'; load({ screen: { Size: [800, 600], Scale: 'Fit' }, theme: {}, styles: {}, roots: [] }); } },
    save: () => download(fileName, Model.exportYaml(state)),
    styles: () => download('styles.yml', Model.exportYaml({ ...state, roots: [] }).replace(/^Screen:\n(  .*\n)*/, '')),
    resetTheme: () => { snapshot(); delete state.theme[themeSel]; renderTheme(); renderStage(); renderYaml(); },
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

  function download(name, text) {
    const a = h('a', { href: URL.createObjectURL(new Blob([text], { type: 'text/yaml' })), download: name });
    a.click(); URL.revokeObjectURL(a.href);
    status(`${name} téléchargé.`);
  }

  // --- Événements ---
  $('palette').append(...Object.keys(Model.TYPES).map(t => h('button', { onclick: () => add(t) }, t)));
  document.querySelectorAll('[data-cmd]').forEach(b => b.addEventListener('click', e => { e.preventDefault(); commands[b.dataset.cmd](); }));
  $('stageWrap').onpointerdown = () => select(null);
  $('styleList').onchange = e => { styleSel = e.target.value; renderStyles(); };
  $('themeList').onchange = e => { themeSel = e.target.value; renderTheme(); };
  ['zoom', 'grid', 'base'].forEach(id => $(id).addEventListener('input', renderStage));
  ['sw', 'sh'].forEach((id, i) => $(id).addEventListener('change', e => { snapshot(); state.screen.Size[i] = Math.max(1, +e.target.value || 1); renderStage(); renderYaml(); }));
  $('scale').onchange = e => { snapshot(); state.screen.Scale = e.target.value; renderYaml(); };
  $('yaml').addEventListener('blur', renderYaml);
  $('open').onchange = async e => {
    const file = e.target.files[0]; if (!file) return;
    try { snapshot(); load(Model.importYaml(await file.text())); fileName = file.name; status(`${file.name} ouvert.`); }
    catch (err) { history.pop(); status(err.message, true); }
    e.target.value = '';
  };
  addEventListener('keydown', e => {
    if (/INPUT|TEXTAREA|SELECT/.test(document.activeElement?.tagName)) return;
    const ctrl = e.ctrlKey || e.metaKey, el = elAt(sel);
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
