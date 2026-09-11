/**
 * ContextLab — Topic Constellation Canvas Visualizer
 * Renders an interactive cosmic knowledge graph of Embrapa research themes.
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

    this.categoryColors = {
      'Sistemas Produtivos': { bg: '#28dea0', glow: 'rgba(40, 222, 160, 0.4)' },
      'Clima & Sustentabilidade': { bg: '#3898ff', glow: 'rgba(56, 152, 255, 0.4)' },
      'Agro Digital': { bg: '#a855f7', glow: 'rgba(168, 85, 247, 0.4)' },
      'Biotecnologia': { bg: '#e9b348', glow: 'rgba(233, 179, 72, 0.4)' },
      'Biomas & Ecologia': { bg: '#06b6d4', glow: 'rgba(6, 182, 212, 0.4)' },
      'Governança Epistêmica': { bg: '#f43f5e', glow: 'rgba(244, 63, 94, 0.4)' },
      'default': { bg: '#94a3b8', glow: 'rgba(148, 163, 184, 0.3)' }
    };

    if (this.canvas) {
      this.ctx = this.canvas.getContext('2d');
      this.initEvents();
      this.resize();
    }
  }

  resize() {
    if (!this.canvas || !this.container) return;
    const rect = this.container.getBoundingClientRect();
    const dpr = window.devicePixelRatio || 1;
    this.width = rect.width;
    this.height = rect.height || 280;
    this.canvas.width = this.width * dpr;
    this.canvas.height = this.height * dpr;
    this.ctx.scale(dpr, dpr);
    this.layoutNodes();
  }

  initEvents() {
    window.addEventListener('resize', () => this.resize());

    this.canvas.addEventListener('mousemove', (e) => {
      const rect = this.canvas.getBoundingClientRect();
      const x = e.clientX - rect.left;
      const y = e.clientY - rect.top;
      this.handleMouseMove(x, y, e);
    });

    this.canvas.addEventListener('mouseleave', () => {
      this.hoveredNode = null;
      if (this.tooltip) this.tooltip.classList.remove('visible');
    });

    this.canvas.addEventListener('click', () => {
      if (this.hoveredNode) {
        this.selectNode(this.hoveredNode);
      }
    });
  }

  async loadData() {
    try {
      const data = await api.getTopicGraph();
      state.setTopicGraphData(data);
      this.nodes = (data.nodes || []).map(n => ({
        ...n,
        x: 0,
        y: 0,
        vx: (Math.random() - 0.5) * 0.4,
        vy: (Math.random() - 0.5) * 0.4,
        radius: Math.max(14, Math.min(32, 12 + (n.count || 1) * 1.5))
      }));
      this.edges = data.edges || [];
      this.layoutNodes();
      this.startAnimation();
      this.updateMetrics(data);
    } catch (err) {
      console.warn('Could not load topic graph:', err);
    }
  }

  layoutNodes() {
    if (!this.width || !this.height || !this.nodes.length) return;
    const centerX = this.width / 2;
    const centerY = this.height / 2;
    const angleStep = (2 * Math.PI) / this.nodes.length;

    this.nodes.forEach((node, i) => {
      const dist = Math.min(centerX, centerY) * 0.65 + (i % 2 === 0 ? 25 : -25);
      const angle = i * angleStep;
      node.x = centerX + Math.cos(angle) * dist;
      node.y = centerY + Math.sin(angle) * dist;
    });
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

    // Filter documents or execute search
    const searchInput = document.getElementById('qa-search-input') || document.getElementById('global-search-input');
    if (searchInput) {
      searchInput.value = node.label;
      const searchBtn = document.getElementById('btn-submit-qa');
      if (searchBtn) searchBtn.click();
    }
  }

  handleMouseMove(x, y, event) {
    let found = null;
    for (const node of this.nodes) {
      const dx = x - node.x;
      const dy = y - node.y;
      if (Math.hypot(dx, dy) <= node.radius + 4) {
        found = node;
        break;
      }
    }

    this.hoveredNode = found;
    this.canvas.style.cursor = found ? 'pointer' : 'default';

    if (found && this.tooltip) {
      const colors = this.categoryColors[found.category] || this.categoryColors.default;
      this.tooltip.innerHTML = `
        <div style="font-weight:700; font-size:0.85rem; color:#fff; display:flex; align-items:center; gap:0.4rem;">
          <span style="display:inline-block; width:8px; height:8px; border-radius:50%; background:${colors.bg}"></span>
          ${found.label}
        </div>
        <div style="color:#94a3b8; font-size:0.7rem; margin-top:0.2rem;">${found.category}</div>
        <div style="margin-top:0.4rem; font-size:0.72rem; color:#3898ff;">
          ${found.count} documentos relacionados ${found.is_user_interest ? '• <span style="color:#28dea0">★ Sua Pesquisa</span>' : ''}
        </div>
      `;
      this.tooltip.style.left = `${x}px`;
      this.tooltip.style.top = `${y - 10}px`;
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

      // 1. Draw subtle background coordinate mesh & orbital rings
      this.ctx.strokeStyle = 'rgba(56, 152, 255, 0.06)';
      this.ctx.lineWidth = 1;
      this.ctx.beginPath();
      this.ctx.arc(this.width / 2, this.height / 2, Math.min(this.width, this.height) * 0.4, 0, Math.PI * 2);
      this.ctx.stroke();

      this.ctx.beginPath();
      this.ctx.arc(this.width / 2, this.height / 2, Math.min(this.width, this.height) * 0.68, 0, Math.PI * 2);
      this.ctx.stroke();

      // 2. Physics & drift simulation
      this.nodes.forEach(n => {
        n.x += n.vx + Math.sin(time + n.radius) * 0.15;
        n.y += n.vy + Math.cos(time + n.radius) * 0.15;

        // Keep inside bounds
        const pad = n.radius + 15;
        if (n.x < pad) { n.x = pad; n.vx *= -1; }
        if (n.x > this.width - pad) { n.x = this.width - pad; n.vx *= -1; }
        if (n.y < pad) { n.y = pad; n.vy *= -1; }
        if (n.y > this.height - pad) { n.y = this.height - pad; n.vy *= -1; }
      });

      // 3. Draw Edges
      const nodeMap = new Map(this.nodes.map(n => [n.id, n]));
      this.edges.forEach(edge => {
        const src = nodeMap.get(edge.source);
        const tgt = nodeMap.get(edge.target);
        if (!src || !tgt) return;

        const isHighlighted = (this.hoveredNode && (this.hoveredNode.id === src.id || this.hoveredNode.id === tgt.id));
        const alpha = isHighlighted ? 0.65 : 0.18;
        const color = isHighlighted ? '#3898ff' : 'rgba(148, 163, 184, 0.2)';

        this.ctx.beginPath();
        this.ctx.moveTo(src.x, src.y);
        this.ctx.lineTo(tgt.x, tgt.y);
        this.ctx.strokeStyle = isHighlighted ? '#3898ff' : `rgba(56, 152, 255, ${alpha})`;
        this.ctx.lineWidth = isHighlighted ? 2 : Math.max(1, edge.weight * 0.6);
        this.ctx.stroke();
      });

      // 4. Draw Nodes
      this.nodes.forEach(node => {
        const matchesFilter = (this.activeFilter === 'all' || node.category === this.activeFilter);
        const isHovered = (this.hoveredNode && this.hoveredNode.id === node.id);
        const isSelected = (this.selectedNode && this.selectedNode.id === node.id);
        const colors = this.categoryColors[node.category] || this.categoryColors.default;

        const opacity = matchesFilter ? 1 : 0.25;

        // Glow ring for user interest or hovered
        if (node.is_user_interest || isHovered || isSelected) {
          this.ctx.beginPath();
          this.ctx.arc(node.x, node.y, node.radius + (isHovered ? 8 : 4) + Math.sin(time * 3) * 2, 0, Math.PI * 2);
          this.ctx.fillStyle = colors.glow;
          this.ctx.fill();
        }

        // Main node circle
        this.ctx.beginPath();
        this.ctx.arc(node.x, node.y, node.radius, 0, Math.PI * 2);
        this.ctx.fillStyle = matchesFilter ? colors.bg : 'rgba(71, 85, 105, 0.4)';
        this.ctx.globalAlpha = opacity;
        this.ctx.fill();

        this.ctx.strokeStyle = '#ffffff';
        this.ctx.lineWidth = isHovered ? 2.5 : 1.2;
        this.ctx.stroke();
        this.ctx.globalAlpha = 1;

        // Node Label
        this.ctx.font = `${isHovered ? '600 11px' : '500 10px'} "Inter", sans-serif`;
        this.ctx.fillStyle = matchesFilter ? '#f8fafc' : '#64748b';
        this.ctx.textAlign = 'center';
        this.ctx.fillText(node.label, node.x, node.y + node.radius + 13);
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
