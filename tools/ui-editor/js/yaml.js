// Mini YAML (même sous-ensemble que la lib C++ SigmaYaml) : maps, séquences, [..], {..}, "..", '..', commentaires.
const Yaml = (() => {
  const trim = s => s.replace(/^[ \t\r]+|[ \t\r]+$/g, '');
  const isSeq = t => t === '-' || t.startsWith('- ');
  const fail = (msg, ln) => { throw new Error(`ligne ${ln} : ${msg}`); };

  function stripComment(line) {
    let q = '';
    for (let i = 0; i < line.length; i++) {
      const c = line[i];
      if (q) { if (q === '"' && c === '\\') i++; else if (c === q) q = ''; }
      else if ((c === '"' || c === "'") && (i === 0 || ' [{,:-'.includes(line[i - 1]))) q = c;
      else if (c === '#' && (i === 0 || line[i - 1] === ' ' || line[i - 1] === '\t')) return line.slice(0, i);
    }
    return line;
  }

  function keyColon(t) {
    if (!t || t[0] === '[' || t[0] === '{') return -1;
    let q = '';
    for (let i = 0; i < t.length; i++) {
      const c = t[i];
      if (q) { if (q === '"' && c === '\\') i++; else if (c === q) q = ''; }
      else if ((c === '"' || c === "'") && i === 0) q = c;
      else if (c === ':' && (i + 1 === t.length || t[i + 1] === ' ')) return i;
    }
    return -1;
  }

  function inline(text, ln) {
    let p = 0;
    const peek = () => text[p] || '';
    const ws = () => { while (p < text.length && (text[p] === ' ' || text[p] === '\t')) p++; };
    const expect = c => { ws(); if (peek() !== c) fail(`'${c}' attendu`, ln); p++; };
    const quoted = () => {
      const q = text[p++]; let out = '';
      while (p < text.length) {
        const c = text[p++];
        if (q === "'") { if (c !== "'") { out += c; continue; } if (peek() !== "'") return out; out += "'"; p++; continue; }
        if (c === '"') return out;
        if (c !== '\\') { out += c; continue; }
        const e = text[p++];
        if (e === 'u') { out += String.fromCharCode(parseInt(text.substr(p, 4), 16)); p += 4; }
        else out += { n: '\n', t: '\t', r: '\r', 0: '\0' }[e] ?? e;
      }
      fail('chaîne non terminée', ln);
    };
    const plain = (flow, key) => {
      const b = p;
      while (p < text.length && !(flow && ',]}'.includes(text[p])) && !(key && text[p] === ':')) p++;
      return trim(text.slice(b, p));
    };
    const value = flow => {
      ws();
      const c = peek();
      if (c === '[') {
        p++; const a = []; ws();
        if (peek() === ']') { p++; return a; }
        for (;;) { a.push(value(true)); ws(); if (peek() === ',') { p++; continue; } expect(']'); return a; }
      }
      if (c === '{') {
        p++; const o = {}; ws();
        if (peek() === '}') { p++; return o; }
        for (;;) {
          ws(); const k = (peek() === '"' || peek() === "'") ? quoted() : plain(true, true);
          expect(':'); o[k] = value(true); ws();
          if (peek() === ',') { p++; continue; } expect('}'); return o;
        }
      }
      if (c === '"' || c === "'") return quoted();
      const s = plain(flow, false);
      return (s === '' || s === '~' || /^null$/i.test(s)) ? null : s;
    };
    const v = value(false); ws();
    if (p !== text.length) fail('caractères inattendus après la valeur', ln);
    return v;
  }

  function parse(src) {
    const L = [];
    src.replace(/^﻿/, '').split('\n').forEach((raw, i) => {
      const line = stripComment(raw), ind = line.search(/[^ ]/);
      if (ind < 0) return;
      if (line[ind] === '\t') fail("tabulation interdite dans l'indentation", i + 1);
      const t = trim(line.slice(ind));
      if (t && !(ind === 0 && (t === '---' || t === '...'))) L.push({ ind, t, ln: i + 1 });
    });
    let k = 0;
    const block = ind => isSeq(L[k].t) ? seq(ind) : keyColon(L[k].t) >= 0 ? map(ind) : inline(L[k].t, L[k++].ln);
    const nested = (ind, sameSeq) => {
      if (k >= L.length) return null;
      if (L[k].ind > ind) return block(L[k].ind);
      return sameSeq && L[k].ind === ind && isSeq(L[k].t) ? seq(ind) : null;
    };
    const seq = ind => {
      const a = [];
      while (k < L.length && L[k].ind >= ind && isSeq(L[k].t)) {
        const l = L[k];
        if (l.ind > ind) fail('indentation inattendue', l.ln);
        const s = l.t.slice(1).search(/[^ ]/) + 1;
        if (s === 0) { k++; a.push(nested(ind, false)); continue; }
        const c = l.t.slice(s);
        if (isSeq(c) || keyColon(c) >= 0) { l.ind = ind + s; l.t = c; a.push(block(l.ind)); continue; }
        k++; a.push(inline(c, l.ln));
      }
      return a;
    };
    const map = ind => {
      const o = {};
      while (k < L.length && L[k].ind >= ind && !isSeq(L[k].t)) {
        const l = L[k];
        if (l.ind > ind) fail('indentation inattendue', l.ln);
        const c = keyColon(l.t);
        if (c < 0) fail("'clé: valeur' attendu", l.ln);
        let key = trim(l.t.slice(0, c));
        if (key[0] === '"' || key[0] === "'") key = inline(key, l.ln);
        if (key in o) fail(`clé dupliquée '${key}'`, l.ln);
        const rest = trim(l.t.slice(c + 1));
        k++;
        o[key] = rest === '' ? nested(ind, true) : inline(rest, l.ln);
      }
      return o;
    };
    if (!L.length) return null;
    const root = block(L[0].ind);
    if (k < L.length) fail('indentation incohérente', L[k].ln);
    return root;
  }

  const needsQuote = s => s === '' || s === '~' || s === 'null' || s === '-' || /^[ ?:,\[\]{}#&*!|>'"%@`]|^- | $|:$|: | #|[,\[\]{}"\x00-\x1f]/.test(s);
  const scalar = v => {
    if (v === null || v === undefined) return '~';
    if (typeof v !== 'string') return String(v);
    return needsQuote(v) ? JSON.stringify(v) : v;
  };
  const isObj = v => v && typeof v === 'object' && !Array.isArray(v);
  const isInline = v => !v || typeof v !== 'object' || (Array.isArray(v) ? !v.length || (v.every(x => !x || typeof x !== 'object') && v.join(', ').length <= 80) : !Object.keys(v).length);
  const inl = v => Array.isArray(v) ? `[${v.map(inl).join(', ')}]` : isObj(v) ? '{}' : scalar(v);

  function dump(v, ind = 0) {
    const pad = ' '.repeat(ind);
    if (isObj(v)) return Object.entries(v).filter(([, x]) => x !== undefined)
      .map(([key, x]) => `${pad}${scalar(key)}:` + (isInline(x) ? ` ${inl(x)}\n` : `\n${dump(x, ind + 2)}`)).join('');
    if (Array.isArray(v)) return v.map(x => isInline(x) ? `${pad}- ${inl(x)}\n` : pad + '- ' + dump(x, ind + 2).slice(ind + 2)).join('');
    return `${pad}${inl(v)}\n`;
  }

  return { parse, dump };
})();
