// ==========================================================================
// ContextLab — Interactive Semantic Graph (WEB-EXPERIENCE-001)
// ==========================================================================

export function renderSemanticGraph(containerId, relations = [], documents = []) {
  const container = document.getElementById(containerId);
  if (!container) return;

  const width = container.clientWidth || 250;
  const height = container.clientHeight || 180;
  const cx = width / 2;
  const cy = height / 2;

  // Build nodes from relations or canonical default structure
  let nodes = [];
  let edges = [];

  if (relations && relations.length > 0) {
    // Center node is ContextLab or primary project
    nodes.push({ id: 'center', label: 'ContextLab', sub: '(projeto)', type: 'project', x: cx, y: cy, color: 'var(--cl-green)' });

    const radius = Math.min(width, height) * 0.38;
    const count = Math.min(relations.length, 6);
    const angleStep = (2 * Math.PI) / count;

    for (let i = 0; i < count; i++) {
      const r = relations[i];
      const angle = i * angleStep;
      const nx = cx + radius * Math.cos(angle);
      const ny = cy + radius * Math.sin(angle);

      let type = 'rel';
      let color = 'var(--cl-cyan)';
      let label = r.object || r.subject;
      if (label.length > 14) label = label.substring(0, 12) + '...';

      if (r.predicate === 'isPartOf') { type = 'concept'; color = 'var(--cl-blue)'; }
      else if (r.predicate === 'concerns') { type = 'exp'; color = 'var(--cl-gold)'; }
      else if (r.predicate === 'isVersionOf') { type = 'doc'; color = 'var(--cl-green)'; }

      nodes.push({ id: `n_${i}`, label: label, sub: `(${r.predicate})`, type, x: nx, y: ny, color });
      edges.push({ x1: cx, y1: cy, x2: nx, y2: ny, color: 'rgba(130, 183, 224, 0.3)' });
    }
  } else {
    // Canonical reference graph
    nodes = [
      { id: 'center', label: 'ContextLab', sub: '(projeto)', type: 'project', x: cx, y: cy, color: '#28dea0' },
      { id: 'n1', label: 'artigo-exemplo.pdf', sub: '(documento)', type: 'doc', x: cx - 65, y: cy - 45, color: '#28dea0' },
      { id: 'n2', label: 'proposta.pdf', sub: '(relacionado)', type: 'rel', x: cx + 65, y: cy - 45, color: '#36c7ff' },
      { id: 'n3', label: 'metadados', sub: '(conceito)', type: 'concept', x: cx - 75, y: cy + 15, color: '#3898ff' },
      { id: 'n4', label: 'proveniência', sub: '(conceito)', type: 'concept', x: cx + 75, y: cy + 15, color: '#3898ff' },
      { id: 'n5', label: 'RES-SAIT', sub: '(experimento)', type: 'exp', x: cx - 60, y: cy + 55, color: '#e9b348' },
      { id: 'n6', label: 'autoingestão', sub: '(experimento)', type: 'exp', x: cx, y: cy + 62, color: '#e9b348' },
      { id: 'n7', label: 'aprofundamento', sub: '(experimento)', type: 'exp', x: cx + 60, y: cy + 55, color: '#e9b348' }
    ];

    for (let i = 1; i < nodes.length; i++) {
      edges.push({ x1: cx, y1: cy, x2: nodes[i].x, y2: nodes[i].y, color: 'rgba(56, 152, 255, 0.25)' });
    }
  }

  // Generate SVG
  let edgesSvg = edges.map(e => `
    <line x1="${e.x1}" y1="${e.y1}" x2="${e.x2}" y2="${e.y2}" stroke="${e.color}" stroke-width="1.5" stroke-dasharray="3 3" />
  `).join('');

  let nodesSvg = nodes.map(n => {
    const isCenter = n.type === 'project';
    const r = isCenter ? 18 : 10;
    return `
      <g class="graph-node-g" transform="translate(${n.x}, ${n.y})" style="cursor: pointer;">
        <circle r="${r}" fill="${n.color}" fill-opacity="0.8" stroke="#ffffff" stroke-width="${isCenter ? 2 : 1}" filter="drop-shadow(0 0 8px ${n.color})"/>
        <text y="${r + 10}" fill="#f4f7fb" font-size="8" font-weight="600" text-anchor="middle" font-family="Inter, sans-serif">${n.label}</text>
        <text y="${r + 18}" fill="#8296a8" font-size="7" text-anchor="middle" font-family="JetBrains Mono, monospace">${n.sub}</text>
      </g>
    `;
  }).join('');

  container.innerHTML = `
    <svg width="100%" height="100%" viewBox="0 0 ${width} ${height}">
      ${edgesSvg}
      ${nodesSvg}
    </svg>
  `;
}
