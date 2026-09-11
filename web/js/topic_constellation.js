/**
 * ContextLab — Topic Constellation Canvas Visualizer (Refactored Epistemic Abstraction)
 * Renders a clean, high-level, interactive epistemic map with non-overlapping nodes and clear readability.
 */

import { api } from '../api.js';
import { state } from '../state.js';

export class TopicConstellation {
  constructor(containerId, options = {}) {
    this.container = document.getElementById(containerId);
    this.canvas = document.getElementById('topic-constellation-canvas');
    this.tooltip = document.getElementById('constellation-tooltip');
    this.options = options;

    this.nodes = [];
    this.edges = [];
    this.activeFilter = 'all';
    this.hoveredNode = null;
    this.selectedNode = null;
    this.animFrameId = null;
    this.simulationSteps = 0;

    this.categoryColors = {
      'Projetos': { bg: '#28dea0', glow: 'rgba(40, 222, 160, 0.45)', border: '#28dea0' },
      'Domínios de Pesquisa': { bg: '#3898ff', glow: 'rgba(56, 152, 255, 0.45)', border: '#3898ff' },
      'Métodos & Estratégias': { bg: '#a855f7', glow: 'rgba(168, 85, 247, 0.45)', border: '#a855f7' },
      'Metadados & Governança': { bg: '#f43f5e', glow: 'rgba(244, 63, 94, 0.45)', border: '#f43f5e' },
      'Artefatos de Pesquisa': { bg: '#e9b348', glow: 'rgba(233, 179, 72, 0.45)', border: '#e9b348' },
      'Conceitos de Pesquisa': { bg: '#818cf8', glow: 'rgba(129, 140, 248, 0.45)', border: '#818cf8' },
      'Conceitos': { bg: '#10b981', glow: 'rgba(16, 185, 129, 0.45)', border: '#10b981' },
      'default': { bg: '#94a3b8', glow: 'rgba(148, 163, 184, 0.35)', border: '#94a3b8' }
    };

    if (this.canvas) {
      this.ctx = this.canvas.getContext('2d');
      this.initEvents();
      this.resize();
    }
  }

  getCategoryColor(cat) {
    if (!cat) return this.categoryColors.default;
    return this.categoryColors[cat] || this.categoryColors['default'];
  }

  resize() {
    if (!this.canvas || !this.container) return;
    const rect = this.container.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    this.width = rect.width;
    this.height = rect.height || 300;
    this.canvas.width = this.width * dpr;
    this.canvas.height = this.height * dpr;
    this.ctx.scale(dpr, dpr);
    this.layoutNodes();
  }

  initEvents() {
    window.addEventListener('resize', () => this.resize());

    this.canvas?.addEventListener('mousemove', (e) => {
      const rect = this.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;
      this.handleMouseMove(x, y);
    });

    this.canvas?.addEventListener('mouseleave', () => {
      this.hoveredNode = null;
      if (this.tooltip) this.tooltip.classList.remove('visible');
    });

    this.canvas?.addEventListener('click', () => {
      if (this.hoveredNode) {
        this.selectNode(this.hoveredNode);
      }
    });
  }

  async loadData() {
    try {
      const data = await api.getTopicGraph();
      state.setTopicGraphData(data);
      
      const rawNodes = data.nodes || [];
      this.nodes = rawNodes.map((n, i) => {
        let baseRadius = 14;
        if (n.category === 'Projetos') baseRadius = 22;
        else if (n.category === 'Domínios de Pesquisa') baseRadius = 18;
        else if (n.category === 'Métodos & Estratégias') baseRadius = 16;

        return {
          ...n,
          x: 0,
          y: 0,
          vx: 0,
          vy: 0,
          radius: baseRadius + Math.min(6, (n.count || 1)),
          index: i
        };
      });

      this.edges = data.edges || [];
      this.layoutNodes();
      this.renderDynamicFiltersAndLegend();
      this.startAnimation();
      this.updateMetrics(data);
    } catch (err) {
      console.warn('Could not load topic graph:', err);
    }
  }

  renderDynamicFiltersAndLegend() {
    const filtersContainer = document.querySelector('.cl-constellation-filters');
    const legendContainer = document.querySelector('.cl-legend-items');

    if (!filtersContainer && !legendContainer) return;

    const catCounts = {};
    this.nodes.forEach(n => {
      const c = n.category || 'Outros';
      catCounts[c] = (catCounts[c] || 0) + 1;
    });

    const categories = Object.keys(catCounts);

    if (filtersContainer) {
      filtersContainer.innerHTML = '';
      const allBtn = document.createElement('button');
      allBtn.className = `cl-topic-filter-btn ${this.activeFilter === 'all' ? 'active' : ''}`;
      allBtn.dataset.cat = 'all';
      allBtn.textContent = `Todos (${this.nodes.length})`;
      allBtn.addEventListener('click', () => this.setFilter('all'));
      filtersContainer.appendChild(allBtn);

      categories.forEach(cat => {
        const btn = document.createElement('button');
        btn.className = `cl-topic-filter-btn ${this.activeFilter === cat ? 'active' : ''}`;
        btn.dataset.cat = cat;
        btn.textContent = `${cat} (${catCounts[cat]})`;
        btn.addEventListener('click', () => this.setFilter(cat));
        filtersContainer.appendChild(btn);
      });
    }

    if (legendContainer) {
      legendContainer.innerHTML = '';
      categories.forEach(cat => {
        const color = this.getCategoryColor(cat);
        const item = document.createElement('div');
        item.className = 'cl-legend-item';
        item.innerHTML = `<span class="cl-legend-dot" style="background:${color.bg}"></span> ${cat}`;
        legendContainer.appendChild(item);
      });
    }
  }

  layoutNodes() {
    if (!this.width || !this.height || !this.nodes.length) return;
    const centerX = this.width / 2;
    const centerY = this.height / 2;
    const count = this.nodes.length;
    const angleStep = (2 * Math.PI) / count;

    this.nodes.forEach((node, i) => {
      // Position Projects near center, domains in middle orbit, concepts outer orbit
      let dist = Math.min(centerX, centerY) * 0.55;
      if (node.category === 'Projetos') {
        dist = Math.min(centerX, centerY) * 0.25;
      } else if (node.category === 'Domínios de Pesquisa') {
        dist = Math.min(centerX, centerY) * 0.48;
      } else {
        dist = Math.min(centerX, centerY) * 0.72;
      }

      const angle = i * angleStep;
      node.x = centerX + Math.cos(angle) * dist;
      node.y = centerY + Math.sin(angle) * dist;
      node.vx = 0;
      node.vy = 0;
    });

    this.simulationSteps = 0;
  }

  setFilter(category) {
    this.activeFilter = category;
    document.querySelectorAll('.cl-topic-filter-btn').forEach(btn => {
      btn.classList.toggle('active', btn.dataset.cat === category);
    });
  }

  selectNode(node) {
    this.selectedNode = node;
    state.setSelectedTopic(node.label);

    const searchInput = document.getElementById('qa-search-input') || document.getElementById('global-search-input');
    if (searchInput) {
      searchInput.value = node.label;
      const searchBtn = document.getElementById('btn-submit-qa');
      if (searchBtn) searchBtn.click();
    }
  }

  handleMouseMove(x, y) {
    let found = null;
    for (const node of this.nodes) {
      const dx = x - node.x;
      const dy = y - node.y;
      if (Math.hypot(dx, dy) <= node.radius + 8) {
        found = node;
        break;
      }
    }

    this.hoveredNode = found;
    if (this.canvas) this.canvas.style.cursor = found ? 'pointer' : 'default';

    if (found && this.tooltip) {
      const colors = this.getCategoryColor(found.category);
      this.tooltip.innerHTML = `
        <div style="font-weight:700; font-size:0.85rem; color:#fff; display:flex; align-items:center; gap:0.4rem;">
          <span style="display:inline-block; width:8px; height:8px; border-radius:50%; background:${colors.bg}"></span>
          ${found.label}
        </div>
        <div style="color:#94a3b8; font-size:0.7rem; margin-top:0.2rem;">${found.category}</div>
        <div style="margin-top:0.4rem; font-size:0.72rem; color:#3898ff;">
          ${found.count} vínculo(s) computável(is) ${found.is_user_interest ? '• <span style="color:#28dea0">★ Seu Documento</span>' : ''}
        </div>
      `;
      this.tooltip.style.left = `${Math.min(this.width - 200, Math.max(10, x))}px`;
      this.tooltip.style.top = `${Math.max(10, y - 20)}px`;
      this.tooltip.classList.add('visible');
    } else if (this.tooltip) {
      this.tooltip.classList.remove('visible');
    }
  }

  startAnimation() {
    if (this.animFrameId) cancelAnimationFrame(this.animFrameId);
    let time = 0;

    const render = () => {
      time += 0.02;
      this.ctx.clearRect(0, 0, this.width, this.height);

      if (this.nodes.length === 0) {
        this.ctx.font = '500 13px "Inter", sans-serif';
        this.ctx.fillStyle = '#64748b';
        this.ctx.textAlign = 'center';
        this.ctx.fillText('Nenhum conceito indexado. Ingeste documentos para sintetizar a constelação.', this.width / 2, this.height / 2);
        return;
      }

      const centerX = this.width / 2;
      const centerY = this.height / 2;

      // 1. Orbital Background Rings
      this.ctx.strokeStyle = 'rgba(56, 152, 255, 0.05)';
      this.ctx.lineWidth = 1;
      this.ctx.beginPath();
      this.ctx.arc(centerX, centerY, Math.min(this.width, this.height) * 0.28, 0, Math.PI * 2);
      this.ctx.stroke();

      this.ctx.beginPath();
      this.ctx.arc(centerX, centerY, Math.min(this.width, this.height) * 0.52, 0, Math.PI * 2);
      this.ctx.stroke();

      // 2. Physics Simulation with Repulsion and Soft Damping
      const nodeMap = new Map(this.nodes.map(n => [n.id, n]));

      if (this.simulationSteps < 150) {
        this.simulationSteps++;

        // Node-node repulsion
        for (let i = 0; i < this.nodes.length; i++) {
          for (let j = i + 1; j < this.nodes.length; j++) {
            const n1 = this.nodes[i];
            const n2 = this.nodes[j];
            const dx = n2.x - n1.x;
            const dy = n2.y - n1.y;
            const dist = Math.hypot(dx, dy) || 1;
            const minDist = (n1.radius + n2.radius) * 3.2;

            if (dist < minDist) {
              const force = ((minDist - dist) / minDist) * 0.4;
              const fx = (dx / dist) * force;
              const fy = (dy / dist) * force;
              n1.vx -= fx;
              n1.vy -= fy;
              n2.vx += fx;
              n2.vy += fy;
            }
          }
        }

        // Edge spring attraction
        this.edges.forEach(edge => {
          const src = nodeMap.get(edge.source);
          const tgt = nodeMap.get(edge.target);
          if (!src || !tgt) return;

          const dx = tgt.x - src.x;
          const dy = tgt.y - src.y;
          const dist = Math.hypot(dx, dy) || 1;
          const targetDist = 110;
          const springForce = (dist - targetDist) * 0.005;

          const fx = (dx / dist) * springForce;
          const fy = (dy / dist) * springForce;
          src.vx += fx;
          src.vy += fy;
          tgt.vx -= fx;
          tgt.vy -= fy;
        });

        // Center pull & boundary constraints
        this.nodes.forEach(n => {
          const dxCenter = centerX - n.x;
          const dyCenter = centerY - n.y;
          n.vx += dxCenter * 0.002;
          n.vy += dyCenter * 0.002;

          // Apply velocity with damping
          n.x += n.vx;
          n.y += n.vy;
          n.vx *= 0.88;
          n.vy *= 0.88;

          const pad = n.radius + 20;
          n.x = Math.max(pad, Math.min(this.width - pad, n.x));
          n.y = Math.max(pad, Math.min(this.height - pad, n.y));
        });
      } else {
        // Subtle organic breathing motion once stabilized
        this.nodes.forEach(n => {
          n.x += Math.sin(time + n.index) * 0.08;
          n.y += Math.cos(time + n.index * 1.3) * 0.08;
        });
      }

      // 3. Draw Edges
      this.edges.forEach(edge => {
        const src = nodeMap.get(edge.source);
        const tgt = nodeMap.get(edge.target);
        if (!src || !tgt) return;

        const isHighlighted = (this.hoveredNode && (this.hoveredNode.id === src.id || this.hoveredNode.id === tgt.id));
        const srcMatch = (this.activeFilter === 'all' || src.category === this.activeFilter);
        const tgtMatch = (this.activeFilter === 'all' || tgt.category === this.activeFilter);

        let alpha = 0.15;
        if (isHighlighted) alpha = 0.8;
        else if (srcMatch && tgtMatch) alpha = 0.25;
        else alpha = 0.05;

        this.ctx.beginPath();
        this.ctx.moveTo(src.x, src.y);
        this.ctx.lineTo(tgt.x, tgt.y);
        this.ctx.strokeStyle = isHighlighted ? '#3898ff' : `rgba(56, 152, 255, ${alpha})`;
        this.ctx.lineWidth = isHighlighted ? 2.2 : 1.2;
        this.ctx.stroke();
      });

      // 4. Draw Nodes and High-Contrast Pill Badges
      this.nodes.forEach(node => {
        const matchesFilter = (this.activeFilter === 'all' || node.category === this.activeFilter);
        const isHovered = (this.hoveredNode && this.hoveredNode.id === node.id);
        const isSelected = (this.selectedNode && this.selectedNode.id === node.id);
        const colors = this.getCategoryColor(node.category);

        const opacity = matchesFilter ? 1 : 0.25;

        // Glow ring for highlighted nodes
        if (node.is_user_interest || isHovered || isSelected) {
          this.ctx.beginPath();
          this.ctx.arc(node.x, node.y, node.radius + (isHovered ? 8 : 4) + Math.sin(time * 3) * 2, 0, Math.PI * 2);
          this.ctx.fillStyle = colors.glow;
          this.ctx.fill();
        }

        // Main node body
        this.ctx.beginPath();
        this.ctx.arc(node.x, node.y, node.radius, 0, Math.PI * 2);
        this.ctx.fillStyle = matchesFilter ? colors.bg : 'rgba(71, 85, 105, 0.4)';
        this.ctx.globalAlpha = opacity;
        this.ctx.fill();

        this.ctx.strokeStyle = '#ffffff';
        this.ctx.lineWidth = isHovered ? 2.5 : 1.4;
        this.ctx.stroke();
        this.ctx.globalAlpha = 1;

        // Label Pill Badge (Clean, High-Legibility)
        const labelText = node.label;
        this.ctx.font = `${isHovered ? '600 11px' : '500 10.5px'} "Inter", sans-serif`;
        const textMetrics = this.ctx.measureText(labelText);
        const textWidth = textMetrics.width;
        const pillHeight = 18;
        const pillWidth = textWidth + 12;
        const pillX = node.x - pillWidth / 2;
        const pillY = node.y + node.radius + 4;

        if (matchesFilter) {
          // Pill background
          this.ctx.fillStyle = isHovered ? 'rgba(15, 23, 42, 0.95)' : 'rgba(10, 15, 30, 0.85)';
          this.ctx.strokeStyle = isHovered ? colors.border : 'rgba(255, 255, 255, 0.15)';
          this.ctx.lineWidth = 1;

          this.ctx.beginPath();
          this.ctx.roundRect(pillX, pillY, pillWidth, pillHeight, 9);
          this.ctx.fill();
          this.ctx.stroke();

          // Pill text
          this.ctx.fillStyle = isHovered ? '#ffffff' : '#e2e8f0';
          this.ctx.textAlign = 'center';
          this.ctx.textBaseline = 'middle';
          this.ctx.fillText(labelText, node.x, pillY + pillHeight / 2);
        }
      });

      this.animFrameId = requestAnimationFrame(render);
    };

    render();
  }

  updateMetrics(data) {
    const totalTopicsEl = document.getElementById('constellation-total-topics');
    if (totalTopicsEl) totalTopicsEl.textContent = data.total_topics || (data.nodes ? data.nodes.length : 0);
  }

  stop() {
    if (this.animFrameId) cancelAnimationFrame(this.animFrameId);
  }
}
