import { api } from './api.js';
import { state } from './state.js';

// DOM Elements
const views = {
  corpus: document.getElementById('view-corpus'),
  inspector: document.getElementById('view-inspector'),
  relations: document.getElementById('view-relations'),
  schemas: document.getElementById('view-schemas'),
  events: document.getElementById('view-events')
};

const navBtns = document.querySelectorAll('.nav-btn');
const searchInput = document.getElementById('search-input');
const searchModeSelect = document.getElementById('search-mode');
const routingBox = document.getElementById('routing-box');
const searchResultsContainer = document.getElementById('search-results-container');
const docsGrid = document.getElementById('docs-grid');
const uploadModal = document.getElementById('upload-modal');
const dropzone = document.getElementById('dropzone');
const fileInput = document.getElementById('file-input');

// Initialize App
async function initApp() {
  setupEventListeners();
  state.subscribe(render);
  await refreshData();
}

async function refreshData() {
  try {
    const [docs, projs, rels, schemas, evs, sys] = await Promise.all([
      api.listDocuments(),
      api.listProjects(),
      api.listRelations(),
      api.listSchemas(),
      api.getRecentEvents(),
      api.getSystemInfo()
    ]);

    state.setData({
      documents: docs,
      projects: projs,
      relations: rels,
      schemas: schemas,
      events: evs,
      systemInfo: sys
    });
  } catch (err) {
    console.error('Error refreshing ContextLab data:', err);
  }
}

function setupEventListeners() {
  // Navigation
  navBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      const view = btn.dataset.view;
      state.setView(view);
    });
  });

  // Search
  let debounceTimer;
  searchInput.addEventListener('input', (e) => {
    clearTimeout(debounceTimer);
    const q = e.target.value.trim();
    if (!q) {
      state.setSearchResults(null);
      return;
    }
    debounceTimer = setTimeout(async () => {
      try {
        const mode = searchModeSelect.value;
        const res = await api.search(q, mode);
        state.setSearchResults(res);
      } catch (err) {
        console.error('Search error:', err);
      }
    }, 250);
  });

  searchModeSelect.addEventListener('change', () => {
    if (searchInput.value.trim()) {
      searchInput.dispatchEvent(new Event('input'));
    }
  });

  // Upload Modal
  document.getElementById('btn-open-upload')?.addEventListener('click', () => {
    uploadModal.classList.add('active');
  });

  document.getElementById('btn-close-upload')?.addEventListener('click', () => {
    uploadModal.classList.remove('active');
  });

  uploadModal.addEventListener('click', (e) => {
    if (e.target === uploadModal) uploadModal.classList.remove('active');
  });

  dropzone.addEventListener('click', () => fileInput.click());

  dropzone.addEventListener('dragover', (e) => {
    e.preventDefault();
    dropzone.classList.add('dragover');
  });

  dropzone.addEventListener('dragleave', () => dropzone.classList.remove('dragover'));

  dropzone.addEventListener('drop', async (e) => {
    e.preventDefault();
    dropzone.classList.remove('dragover');
    if (e.dataTransfer.files.length > 0) {
      await handleFileUpload(e.dataTransfer.files[0]);
    }
  });

  fileInput.addEventListener('change', async () => {
    if (fileInput.files.length > 0) {
      await handleFileUpload(fileInput.files[0]);
    }
  });
}

async function handleFileUpload(file) {
  const statusEl = document.getElementById('upload-status');
  statusEl.innerHTML = `<span style="color: var(--accent-cyan)">Ingerindo ${file.name}...</span>`;
  try {
    const res = await api.uploadFile(file);
    statusEl.innerHTML = `<span style="color: var(--accent-emerald)">✓ Ingestão concluída com sucesso! (ID: ${res.document_id})</span>`;
    setTimeout(() => {
      uploadModal.classList.remove('active');
      statusEl.innerHTML = '';
      refreshData();
      if (res.document_id) {
        openDocumentInspector(res.document_id);
      }
    }, 1200);
  } catch (err) {
    statusEl.innerHTML = `<span style="color: var(--accent-rose)">Erro: ${err.message}</span>`;
  }
}

function render(s) {
  // Update Nav Buttons
  navBtns.forEach(btn => {
    btn.classList.toggle('active', btn.dataset.view === s.currentView);
  });

  // Toggle View Containers
  Object.keys(views).forEach(vKey => {
    if (views[vKey]) {
      views[vKey].classList.toggle('active', vKey === s.currentView);
    }
  });

  // Update Stats
  if (s.systemInfo) {
    document.getElementById('stat-docs-count').textContent = s.systemInfo.documents_count || s.documents.length;
    document.getElementById('stat-proj-count').textContent = s.systemInfo.projects_count || s.projects.length;
    document.getElementById('stat-rel-count').textContent = s.systemInfo.relations_count || s.relations.length;
    document.getElementById('stat-schema-count').textContent = s.systemInfo.schemas_count || s.schemas.length;
  }

  // Render Views
  if (s.currentView === 'corpus') renderCorpusView(s);
  if (s.currentView === 'inspector' && s.selectedDocId) renderInspectorView(s.selectedDocId);
  if (s.currentView === 'relations') renderRelationsView(s);
  if (s.currentView === 'schemas') renderSchemasView(s);
  if (s.currentView === 'events') renderEventsView(s);
}

function renderCorpusView(s) {
  // Search Routing & Results
  if (s.searchResults) {
    routingBox.style.display = 'flex';
    const route = s.searchResults.route;
    let badgeClass = 'badge-metadata';
    let badgeLabel = 'METADADOS';

    if (route === 'TEXT_INDEX_REQUIRED') {
      badgeClass = 'badge-text';
      badgeLabel = 'TEXTO (FTS5)';
    } else if (route === 'DEEP_ANALYSIS_REQUIRED') {
      badgeClass = 'badge-deep';
      badgeLabel = 'APROFUNDAMENTO NECESSÁRIO';
    }

    document.getElementById('routing-badge-container').innerHTML = `
      <span class="routing-badge ${badgeClass}">
        <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>
        ${badgeLabel}
      </span>
    `;
    document.getElementById('routing-reason-text').textContent = s.searchResults.reason;

    // Render search matches
    if (s.searchResults.results.length === 0) {
      searchResultsContainer.innerHTML = `<div style="padding: 1.5rem; text-align: center; color: var(--text-muted);">Nenhum resultado encontrado para a consulta.</div>`;
    } else {
      searchResultsContainer.innerHTML = s.searchResults.results.map(r => `
        <div class="doc-card" style="margin-bottom: 1rem; border-left: 3px solid var(--accent-cyan);">
          <div class="doc-header">
            <div>
              <span class="doc-id">${r.document_id}</span>
              <h3 class="doc-title" style="font-size: 1.1rem; margin-top: 0.2rem;">${r.title}</h3>
            </div>
            <span class="tag tag-project">${r.primary_project}</span>
          </div>
          <div style="font-size: 0.85rem; color: var(--accent-emerald); background: rgba(16, 185, 129, 0.08); padding: 0.5rem 0.75rem; border-radius: var(--radius-sm);">
            <strong>Por que apareceu:</strong> ${r.explanation}
          </div>
          ${r.snippet ? `<div style="font-size: 0.8rem; color: var(--text-secondary); background: var(--bg-surface); padding: 0.5rem; border-radius: var(--radius-sm);">${r.snippet}</div>` : ''}
          <div class="doc-footer">
            <span class="tag tag-status">${r.epistemic_status}</span>
            <button class="btn btn-secondary btn-sm" onclick="window.contextlabInspect('${r.document_id}')">Inspecionar Contexto</button>
          </div>
        </div>
      `).join('');
    }
  } else {
    routingBox.style.display = 'none';
    searchResultsContainer.innerHTML = '';
  }

  // Render Document Grid
  if (s.documents.length === 0) {
    docsGrid.innerHTML = `
      <div style="grid-column: 1/-1; padding: 3rem; text-align: center; background: var(--bg-card); border-radius: var(--radius-lg); border: 1px dashed var(--glass-border);">
        <p style="color: var(--text-secondary); margin-bottom: 1rem;">Nenhum documento carregado no corpus.</p>
        <button class="btn btn-primary" onclick="document.getElementById('btn-open-upload').click()">+ Ingerir Primeiro Documento</button>
      </div>
    `;
    return;
  }

  docsGrid.innerHTML = s.documents.map(d => `
    <div class="doc-card">
      <div>
        <div class="doc-header">
          <span class="doc-id">${d.id}</span>
          <span class="doc-version">v${d.version}</span>
        </div>
        <h3 class="doc-title" style="margin-top: 0.5rem;">${d.title}</h3>
        ${d.subtitle ? `<p style="font-size: 0.8rem; color: var(--text-muted); margin-top: 0.25rem;">${d.subtitle}</p>` : ''}
      </div>

      <div class="doc-meta-tags">
        <span class="tag tag-project">${d.primary_project}</span>
        <span class="tag tag-authority">${d.primary_authority}</span>
        <span class="tag tag-status">${d.epistemic_status || d.document_type}</span>
        <span class="tag ${d.text_analyzed ? 'tag-text-analyzed' : 'tag-text-unanalyzed'}">
          ${d.text_analyzed ? '● APROFUNDADO' : '○ NÃO APROFUNDADO'}
        </span>
      </div>

      <div class="doc-footer">
        <span style="font-size: 0.75rem; color: var(--text-muted);">${d.date_created || ''}</span>
        <button class="btn btn-secondary btn-sm" onclick="window.contextlabInspect('${d.id}')">
          Abrir Contexto
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12h14M12 5l7 7-7 7"/></svg>
        </button>
      </div>
    </div>
  `).join('');
}

window.contextlabInspect = (docId) => {
  state.setView('inspector', docId);
};

async function renderInspectorView(docId) {
  const container = document.getElementById('inspector-content');
  container.innerHTML = `<div style="padding: 2rem; text-align: center; color: var(--text-muted);">Carregando detalhes do documento...</div>`;

  try {
    const doc = await api.getDocument(docId);
    
    container.innerHTML = `
      <div class="inspector-container">
        <div class="inspector-header">
          <div>
            <div style="display: flex; align-items: center; gap: 0.65rem; margin-bottom: 0.35rem;">
              <span class="doc-id" style="font-size: 1rem;">${doc.id}</span>
              <span class="doc-version">v${doc.version}</span>
              <span class="tag tag-authority">${doc.primary_authority}</span>
              <span class="tag ${doc.text_analyzed ? 'tag-text-analyzed' : 'tag-text-unanalyzed'}">
                ${doc.text_analyzed ? '● TEXTO APROFUNDADO' : '○ TEXTO NÃO APROFUNDADO'}
              </span>
            </div>
            <h2 style="font-family: var(--font-display); font-size: 1.4rem; color: var(--text-primary);">${doc.title}</h2>
            ${doc.subtitle ? `<p style="color: var(--text-secondary); margin-top: 0.25rem;">${doc.subtitle}</p>` : ''}
          </div>
          <div style="display: flex; gap: 0.5rem;">
            <button class="btn btn-secondary btn-sm" onclick="window.contextlabBackToCorpus()">← Voltar ao Corpus</button>
            <button class="btn btn-cyan btn-sm" id="btn-deepen-doc">⚡ Aprofundar no Texto</button>
          </div>
        </div>

        <div class="inspector-tabs">
          <button class="inspector-tab active" data-tab="tab-overview">Visão Geral</button>
          <button class="inspector-tab" data-tab="tab-context">Contexto Estruturado</button>
          <button class="inspector-tab" data-tab="tab-provenance">Proveniência</button>
          <button class="inspector-tab" data-tab="tab-relations">Relações</button>
          <button class="inspector-tab" data-tab="tab-epistemic">Epistemologia</button>
          <button class="inspector-tab" data-tab="tab-artifacts">Artefatos</button>
          <button class="inspector-tab" data-tab="tab-text">Texto & Leitura</button>
          <button class="inspector-tab" data-tab="tab-json">JSON Bruto</button>
        </div>

        <div class="inspector-body">
          <!-- Tab 1: Visão Geral -->
          <div class="tab-pane active" id="tab-overview">
            <table class="kv-table">
              <tr><th>Identificador</th><td><code>${doc.id}</code></td></tr>
              <tr><th>Título</th><td>${doc.title}</td></tr>
              <tr><th>Versão</th><td>${doc.version}</td></tr>
              <tr><th>Projeto Primário</th><td>${doc.primary_project}</td></tr>
              <tr><th>Tipo Documental</th><td>${doc.document_type}</td></tr>
              <tr><th>Status Epistemológico</th><td><span class="tag tag-status">${doc.epistemic_status}</span></td></tr>
              <tr><th>Autoridade de Origem</th><td><span class="tag tag-authority">${doc.primary_authority}</span></td></tr>
              <tr><th>Idioma</th><td>${doc.language}</td></tr>
              <tr><th>Data de Criação</th><td>${doc.date_created}</td></tr>
              <tr><th>Data de Ingestão</th><td>${doc.created_at}</td></tr>
              <tr><th>Autodescritivo</th><td>${doc.self_describing ? 'Sim (Context-Metadata Block)' : 'Não'}</td></tr>
              <tr><th>Autoingestão Obrigatória</th><td>${doc.self_consumption_required ? 'Sim (Baseline Gate)' : 'Não'}</td></tr>
            </table>
          </div>

          <!-- Tab 2: Contexto Estruturado -->
          <div class="tab-pane" id="tab-context">
            ${doc.metadata_envelopes && doc.metadata_envelopes.length > 0 ? `
              <div style="display: flex; flex-direction: column; gap: 1rem;">
                ${doc.metadata_envelopes.map(env => `
                  <div style="background: var(--bg-surface); padding: 1rem; border-radius: var(--radius-md); border: 1px solid var(--glass-border);">
                    <div style="display: flex; justify-content: space-between; margin-bottom: 0.75rem;">
                      <div>
                        <strong>Schema:</strong> <code>${env.schema_id}</code> (v${env.schema_version})
                      </div>
                      <span class="tag tag-authority">${env.authority}</span>
                    </div>
                    <pre class="code-block">${JSON.stringify(env.payload, null, 2)}</pre>
                  </div>
                `).join('')}
              </div>
            ` : '<p style="color: var(--text-muted);">Nenhum envelope de metadados associado.</p>'}
          </div>

          <!-- Tab 3: Proveniência -->
          <div class="tab-pane" id="tab-provenance">
            <div style="background: var(--bg-surface); padding: 1.25rem; border-radius: var(--radius-md); border: 1px solid var(--glass-border);">
              <h4 style="color: var(--accent-cyan); margin-bottom: 0.75rem;">Fronteira e Origem do Documento</h4>
              <p style="color: var(--text-secondary); margin-bottom: 1rem;">
                Informações de proveniência formalmente declaradas pelo autor para preservar linhagem sem acoplamento de código.
              </p>
              ${renderProvenanceSection(doc)}
            </div>
          </div>

          <!-- Tab 4: Relações -->
          <div class="tab-pane" id="tab-relations">
            <h4 style="margin-bottom: 0.75rem; color: var(--text-primary);">Grafo de Relações Documentais</h4>
            <div class="graph-container" id="doc-graph-container">
              <svg class="graph-svg" id="doc-graph-svg"></svg>
            </div>
            <div style="margin-top: 1rem;">
              <table class="kv-table">
                <thead>
                  <tr><th>Sujeito</th><th>Predicado</th><th>Objeto</th></tr>
                </thead>
                <tbody>
                  ${(doc.relations || []).map(r => `
                    <tr>
                      <td><code>${r.subject}</code></td>
                      <td><span class="tag tag-project">${r.predicate}</span></td>
                      <td><code>${r.object}</code></td>
                    </tr>
                  `).join('')}
                </tbody>
              </table>
            </div>
          </div>

          <!-- Tab 5: Epistemologia -->
          <div class="tab-pane" id="tab-epistemic">
            <div style="background: var(--bg-surface); padding: 1.25rem; border-radius: var(--radius-md); border: 1px solid var(--glass-border);">
              <h4 style="color: var(--accent-secondary); margin-bottom: 0.5rem;">Status e Invariantes Epistêmicos</h4>
              <p style="color: var(--text-secondary); margin-bottom: 1rem;">
                Camada de declaração de evidência, limites de escopo e condições de falsificação computáveis.
              </p>
              ${renderEpistemicSection(doc)}
            </div>
          </div>

          <!-- Tab 6: Artefatos -->
          <div class="tab-pane" id="tab-artifacts">
            <table class="kv-table">
              <thead>
                <tr><th>Nome Original</th><th>Mídia</th><th>Tamanho</th><th>SHA-256 (CAS Imutável)</th></tr>
              </thead>
              <tbody>
                ${(doc.artifacts || []).map(a => `
                  <tr>
                    <td><strong>${a.original_filename}</strong></td>
                    <td><code>${a.media_type}</code></td>
                    <td>${(a.size_bytes / 1024).toFixed(1)} KB</td>
                    <td><code style="font-size: 0.75rem;">${a.sha256}</code></td>
                  </tr>
                `).join('')}
              </tbody>
            </table>
          </div>

          <!-- Tab 7: Texto & Leitura -->
          <div class="tab-pane" id="tab-text">
            ${doc.text_analysis ? `
              <div style="display: flex; flex-direction: column; gap: 1rem;">
                <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 1rem;">
                  <div class="stat-card">
                    <div class="stat-label">Caracteres</div>
                    <div class="stat-value" style="font-size: 1.4rem;">${doc.text_analysis.character_count}</div>
                  </div>
                  <div class="stat-card">
                    <div class="stat-label">Palavras</div>
                    <div class="stat-value" style="font-size: 1.4rem;">${doc.text_analysis.word_count}</div>
                  </div>
                  <div class="stat-card">
                    <div class="stat-label">Linhas</div>
                    <div class="stat-value" style="font-size: 1.4rem;">${doc.text_analysis.line_count}</div>
                  </div>
                  <div class="stat-card">
                    <div class="stat-label">Seções</div>
                    <div class="stat-value" style="font-size: 1.4rem;">${doc.text_analysis.section_count}</div>
                  </div>
                </div>

                <div style="background: var(--bg-surface); padding: 1rem; border-radius: var(--radius-md); border: 1px solid var(--glass-border);">
                  <h4 style="margin-bottom: 0.5rem; color: var(--accent-cyan);">Termos Frequentes Extraídos</h4>
                  <div style="display: flex; flex-wrap: wrap; gap: 0.4rem;">
                    ${(doc.text_analysis.top_terms || []).map(t => `<span class="tag tag-status">${t}</span>`).join('')}
                  </div>
                </div>

                <div style="background: var(--bg-surface); padding: 1rem; border-radius: var(--radius-md); border: 1px solid var(--glass-border);">
                  <h4 style="margin-bottom: 0.5rem; color: var(--text-primary);">Seções Identificadas</h4>
                  <ul style="padding-left: 1.5rem; color: var(--text-secondary); font-size: 0.85rem;">
                    ${(doc.text_analysis.sections || []).map(s => `<li>${s}</li>`).join('')}
                  </ul>
                </div>

                <div style="background: var(--bg-surface); padding: 1rem; border-radius: var(--radius-md); border: 1px solid var(--glass-border);">
                  <h4 style="margin-bottom: 0.5rem; color: var(--text-muted);">Amostra de Texto Indexado (FTS5)</h4>
                  <p style="font-size: 0.85rem; color: var(--text-secondary); line-height: 1.6;">${doc.text_analysis.sample_preview || ''}</p>
                </div>
              </div>
            ` : `
              <div style="padding: 3rem; text-align: center; background: var(--bg-surface); border-radius: var(--radius-md); border: 1px dashed var(--glass-border);">
                <h4 style="color: var(--text-primary); margin-bottom: 0.5rem;">Texto Não Aprofundado</h4>
                <p style="color: var(--text-secondary); max-width: 500px; margin: 0 auto 1.5rem auto;">
                  O sistema operou sob a hipótese metadata-first. A análise textual profunda e indexação FTS5 permanecem diferidas até a solicitação explícita.
                </p>
                <button class="btn btn-primary" id="btn-deepen-doc-inner">⚡ Executar Aprofundamento Textual Agora</button>
              </div>
            `}
          </div>

          <!-- Tab 8: JSON Bruto -->
          <div class="tab-pane" id="tab-json">
            <pre class="code-block">${JSON.stringify(doc, null, 2)}</pre>
          </div>
        </div>
      </div>
    `;

    // Tab navigation logic
    const tabs = container.querySelectorAll('.inspector-tab');
    const panes = container.querySelectorAll('.tab-pane');
    tabs.forEach(tab => {
      tab.addEventListener('click', () => {
        tabs.forEach(t => t.classList.remove('active'));
        panes.forEach(p => p.classList.remove('active'));
        tab.classList.add('active');
        const target = container.querySelector(`#${tab.dataset.tab}`);
        if (target) target.classList.add('active');
        if (tab.dataset.tab === 'tab-relations') {
          drawDocRelationsGraph(doc);
        }
      });
    });

    // Deepen button action
    const triggerDeepen = async () => {
      try {
        await api.deepen(doc.id);
        renderInspectorView(doc.id);
        refreshData();
      } catch (e) {
        alert('Erro ao aprofundar texto: ' + e.message);
      }
    };

    container.querySelector('#btn-deepen-doc')?.addEventListener('click', triggerDeepen);
    container.querySelector('#btn-deepen-doc-inner')?.addEventListener('click', triggerDeepen);

  } catch (err) {
    container.innerHTML = `<div style="padding: 2rem; color: var(--accent-rose);">Erro ao carregar documento: ${err.message}</div>`;
  }
}

window.contextlabBackToCorpus = () => {
  state.setView('corpus');
};

function renderProvenanceSection(doc) {
  if (!doc.metadata_envelopes || doc.metadata_envelopes.length === 0) return '<p>Sem proveniência.</p>';
  const p = doc.metadata_envelopes[0].payload.provenance;
  if (!p) return '<p>Sem bloco de proveniência formal.</p>';

  return `
    <table class="kv-table">
      <tr><th>Contexto de Origem</th><td>${p.origin_context || '-'}</td></tr>
      <tr><th>Papel do Contexto de Origem</th><td><code>${p.origin_context_role || '-'}</code></td></tr>
      <tr><th>Fronteira de Identidade</th><td>${p.identity_boundary || '-'}</td></tr>
      <tr><th>Base de Origem</th><td>${p.source_basis || '-'}</td></tr>
      <tr><th>Política de Herança</th><td>${p.heritage_policy || '-'}</td></tr>
    </table>
  `;
}

function renderEpistemicSection(doc) {
  if (!doc.metadata_envelopes || doc.metadata_envelopes.length === 0) return '<p>Sem dados epistêmicos.</p>';
  const e = doc.metadata_envelopes[0].payload.epistemic_context;
  if (!e) return '<p>Sem bloco de contexto epistêmico formal.</p>';

  return `
    <table class="kv-table">
      <tr><th>Status</th><td><span class="tag tag-status">${e.status || '-'}</span></td></tr>
      <tr><th>Maturidade da Afirmação</th><td><code>${e.claim_maturity || '-'}</code></td></tr>
      <tr><th>Nível de Evidência</th><td><code>${e.evidence_level || '-'}</code></td></tr>
      <tr><th>Alegação Principal</th><td>${e.main_claim || '-'}</td></tr>
      <tr><th>O que NÃO é alegado</th><td>
        <ul style="padding-left: 1.25rem;">
          ${(e.not_claimed || []).map(item => `<li>${item}</li>`).join('')}
        </ul>
      </td></tr>
    </table>
  `;
}

function drawDocRelationsGraph(doc) {
  const svg = document.getElementById('doc-graph-svg');
  if (!svg) return;
  svg.innerHTML = '';

  const width = svg.clientWidth || 700;
  const height = svg.clientHeight || 450;
  const cx = width / 2;
  const cy = height / 2;

  const relations = doc.relations || [];
  const nodes = [{ id: doc.id, label: doc.id, type: 'center', x: cx, y: cy }];

  const radius = Math.min(width, height) * 0.35;
  const angleStep = (2 * Math.PI) / Math.max(relations.length, 1);

  relations.forEach((r, idx) => {
    const angle = idx * angleStep;
    const nx = cx + radius * Math.cos(angle);
    const ny = cy + radius * Math.sin(angle);
    nodes.push({ id: r.object, label: r.object, predicate: r.predicate, type: 'target', x: nx, y: ny });
  });

  // Render Edges
  let edgesHtml = '';
  for (let i = 1; i < nodes.length; i++) {
    const target = nodes[i];
    edgesHtml += `
      <line x1="${cx}" y1="${cy}" x2="${target.x}" y2="${target.y}" stroke="rgba(99, 102, 241, 0.4)" stroke-width="2" stroke-dasharray="4"/>
      <text x="${(cx + target.x) / 2}" y="${(cy + target.y) / 2 - 6}" fill="var(--accent-cyan)" font-size="11" text-anchor="middle" font-family="var(--font-mono)">${target.predicate || 'rel'}</text>
    `;
  }

  // Render Nodes
  let nodesHtml = '';
  nodes.forEach(n => {
    const isCenter = n.type === 'center';
    const color = isCenter ? 'var(--accent-primary)' : 'var(--accent-secondary)';
    const r = isCenter ? 26 : 18;
    nodesHtml += `
      <g transform="translate(${n.x}, ${n.y})">
        <circle r="${r}" fill="${color}" fill-opacity="0.85" stroke="#FFF" stroke-width="${isCenter ? 2.5 : 1.5}" filter="drop-shadow(0 0 8px ${color})"/>
        <text y="${r + 14}" fill="var(--text-primary)" font-size="11" font-weight="600" text-anchor="middle" font-family="var(--font-mono)">${n.label}</text>
      </g>
    `;
  });

  svg.innerHTML = edgesHtml + nodesHtml;
}

function renderRelationsView(s) {
  const container = document.getElementById('relations-content');
  if (s.relations.length === 0) {
    container.innerHTML = `<div style="padding: 2rem; color: var(--text-muted); text-align: center;">Nenhuma relação registrada no corpus.</div>`;
    return;
  }

  container.innerHTML = `
    <div style="background: var(--bg-card); border: 1px solid var(--glass-border); border-radius: var(--radius-lg); padding: 1.5rem;">
      <h3 style="margin-bottom: 1rem; font-family: var(--font-display);">Relações Registradas no Corpus (${s.relations.length})</h3>
      <table class="kv-table">
        <thead>
          <tr><th>Documento</th><th>Sujeito</th><th>Predicado</th><th>Objeto</th></tr>
        </thead>
        <tbody>
          ${s.relations.map(r => `
            <tr>
              <td><code>${r.document_id}</code></td>
              <td><code>${r.subject}</code></td>
              <td><span class="tag tag-project">${r.predicate}</span></td>
              <td><code>${r.object}</code></td>
            </tr>
          `).join('')}
        </tbody>
      </table>
    </div>
  `;
}

function renderSchemasView(s) {
  const container = document.getElementById('schemas-content');
  container.innerHTML = `
    <div style="background: var(--bg-card); border: 1px solid var(--glass-border); border-radius: var(--radius-lg); padding: 1.5rem;">
      <h3 style="margin-bottom: 1rem; font-family: var(--font-display);">Esquemas Registrados no Schema Registry (${s.schemas.length})</h3>
      <div style="display: flex; flex-direction: column; gap: 1rem;">
        ${s.schemas.map(sc => `
          <div style="background: var(--bg-surface); padding: 1rem; border-radius: var(--radius-md); border: 1px solid var(--glass-border);">
            <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 0.5rem;">
              <span style="font-family: var(--font-mono); font-weight: 700; color: var(--accent-cyan);">${sc.id}</span>
              <span class="tag tag-authority">v${sc.version}</span>
            </div>
            <div style="font-size: 0.8rem; color: var(--text-muted); margin-bottom: 0.75rem;">SHA-256: <code>${sc.digest}</code> · Fonte: ${sc.source_path}</div>
            <pre class="code-block" style="max-height: 200px;">${JSON.stringify(sc.schema_json, null, 2)}</pre>
          </div>
        `).join('')}
      </div>
    </div>
  `;
}

function renderEventsView(s) {
  const container = document.getElementById('events-content');
  container.innerHTML = `
    <div style="background: var(--bg-card); border: 1px solid var(--glass-border); border-radius: var(--radius-lg); padding: 1.5rem;">
      <h3 style="margin-bottom: 1rem; font-family: var(--font-display);">Trilha de Auditoria & Ingestões Recentes</h3>
      <table class="kv-table">
        <thead>
          <tr><th>Timestamp</th><th>Arquivo</th><th>Document ID</th><th>Status</th><th>Mensagem</th></tr>
        </thead>
        <tbody>
          ${s.events.map(ev => `
            <tr>
              <td style="font-size: 0.75rem; color: var(--text-muted);">${ev.timestamp}</td>
              <td><strong>${ev.source_file}</strong></td>
              <td><code>${ev.document_id || '-'}</code></td>
              <td><span class="tag ${ev.status === 'SUCCESS' ? 'tag-authority' : 'tag-text-unanalyzed'}">${ev.status}</span></td>
              <td style="font-size: 0.8rem; color: var(--text-secondary);">${ev.message}</td>
            </tr>
          `).join('')}
        </tbody>
      </table>
    </div>
  `;
}

// Start
document.addEventListener('DOMContentLoaded', initApp);
