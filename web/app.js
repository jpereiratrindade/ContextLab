// ==========================================================================
// ContextLab — Main Application Coordinator (WEB-EXPERIENCE-001)
// ==========================================================================

import { api } from './api.js';
import { state } from './state.js';
import { renderSemanticGraph } from './js/graph.js';
import { renderProvenanceTimeline } from './js/timeline.js';
import { setupExperiments } from './js/experiments.js';
import { handleQuestionQuery } from './js/qa_engine.js';
import { TopicConstellation } from './js/topic_constellation.js';
import { AuthManager } from './js/auth_modal.js';

// DOM Elements
const views = {
  inicio: document.getElementById('view-inicio'),
  corpus: document.getElementById('view-corpus'),
  search: document.getElementById('view-search'),
  inspector: document.getElementById('view-inspector'),
  projects: document.getElementById('view-projects'),
  relations: document.getElementById('view-relations'),
  schemas: document.getElementById('view-schemas'),
  events: document.getElementById('view-events')
};

const navBtns = document.querySelectorAll('.cl-nav-btn');
const globalSearchInput = document.getElementById('global-search-input');
const qaSearchInput = document.getElementById('qa-search-input');
const btnSubmitQa = document.getElementById('btn-submit-qa');
const qaChips = document.querySelectorAll('.cl-chip');

// Ingestion Dropzone Elements
const mainDropzone = document.getElementById('main-dropzone');
const mainFileInput = document.getElementById('main-file-input');
const btnTriggerFilePick = document.getElementById('btn-trigger-file-pick');
const mainUploadStatus = document.getElementById('main-upload-status');

// Modals
const editDocModal = document.getElementById('edit-doc-modal');
const relationModal = document.getElementById('relation-modal');
const projectModal = document.getElementById('project-modal');
const deleteConfirmModal = document.getElementById('delete-confirm-modal');

let pendingDeleteCallback = null;
let constellationInstance = null;
let authManagerInstance = null;

// Initialize Application
async function initApp() {
  setupNavigation();
  setupIngestion();
  setupQaAndSearch();
  setupModals();
  setupExperiments(state);

  try {
    constellationInstance = new TopicConstellation('constellation-container');
    authManagerInstance = new AuthManager({
      onLogin: async () => {
        await refreshData();
        await constellationInstance?.loadData();
      }
    });

    state.subscribe(render);
    await refreshData();
    await constellationInstance?.loadData();
  } catch (err) {
    console.warn('ContextLab background init warning:', err);
  }

  // Canonical Initial Q&A Query
  try {
    await handleQuestionQuery('Qual é o projeto?', '', state);
  } catch (err) {
    console.warn('Initial Q&A query warning:', err);
  }
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

function setupNavigation() {
  navBtns.forEach(btn => {
    btn.addEventListener('click', () => {
      const view = btn.dataset.view;
      if (view) {
        state.setView(view);
      }
    });
  });

  document.getElementById('btn-nav-brand')?.addEventListener('click', (e) => {
    e.preventDefault();
    state.setView('inicio');
  });

  document.getElementById('link-view-all-events')?.addEventListener('click', (e) => {
    e.preventDefault();
    state.setView('events');
  });

  document.getElementById('sphere-ingest')?.addEventListener('click', () => {
    btnTriggerFilePick?.click();
  });

  document.getElementById('sphere-explore')?.addEventListener('click', () => {
    state.setView('relations');
  });

  document.getElementById('sphere-decision')?.addEventListener('click', () => {
    handleQuestionQuery('Por que não devemos misturar metadados declarados e derivados?', '', state);
  });
}

function setupIngestion() {
  btnTriggerFilePick?.addEventListener('click', () => mainFileInput?.click());
  document.getElementById('btn-sidebar-ingest')?.addEventListener('click', () => mainFileInput?.click());
  mainDropzone?.addEventListener('click', () => mainFileInput?.click());

  mainDropzone?.addEventListener('dragover', (e) => {
    e.preventDefault();
    mainDropzone.classList.add('dragover');
  });

  mainDropzone?.addEventListener('dragleave', () => mainDropzone.classList.remove('dragover'));

  mainDropzone?.addEventListener('drop', async (e) => {
    e.preventDefault();
    mainDropzone.classList.remove('dragover');
    if (e.dataTransfer.files.length > 0) {
      await handleFileUpload(e.dataTransfer.files[0]);
    }
  });

  mainFileInput?.addEventListener('change', async () => {
    if (mainFileInput.files.length > 0) {
      await handleFileUpload(mainFileInput.files[0]);
    }
  });

  document.getElementById('btn-sample-docs')?.addEventListener('click', () => {
    state.setView('corpus');
  });
}

async function handleFileUpload(file) {
  if (!mainUploadStatus) return;
  mainUploadStatus.innerHTML = `<span style="color: var(--cl-cyan)">Ingerindo ${file.name}...</span>`;
  try {
    const res = await api.uploadFile(file);
    mainUploadStatus.innerHTML = `<span style="color: var(--cl-green)">✓ Ingestão concluída com sucesso! (ID: ${res.document_id})</span>`;
    setTimeout(async () => {
      mainUploadStatus.innerHTML = '';
      await refreshData();
      if (res.document_id) {
        state.setView('inspector', res.document_id);
      }
    }, 1200);
  } catch (err) {
    mainUploadStatus.innerHTML = `<span style="color: var(--cl-rose)">Erro: ${err.message}</span>`;
  }
}

function setupQaAndSearch() {
  // Q&A Question input
  btnSubmitQa?.addEventListener('click', () => {
    const q = qaSearchInput?.value.trim();
    if (q) handleQuestionQuery(q, '', state);
  });

  qaSearchInput?.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
      const q = qaSearchInput.value.trim();
      if (q) handleQuestionQuery(q, '', state);
    }
  });

  // Example Chips
  qaChips.forEach(chip => {
    chip.addEventListener('click', () => {
      const q = chip.dataset.q;
      if (q) {
        if (qaSearchInput) qaSearchInput.value = q;
        handleQuestionQuery(q, '', state);
      }
    });
  });

  // Global search bar
  let debounceTimer;
  globalSearchInput?.addEventListener('input', (e) => {
    clearTimeout(debounceTimer);
    const q = e.target.value.trim();
    if (!q) {
      state.setSearchResults(null);
      return;
    }
    debounceTimer = setTimeout(async () => {
      try {
        const res = await api.search(q, 'auto');
        state.setSearchResults(res);
        state.setView('search');
      } catch (err) {
        console.error('Search error:', err);
      }
    }, 250);
  });

  // Key shortcut '/'
  window.addEventListener('keydown', (e) => {
    if (e.key === '/' && document.activeElement !== globalSearchInput && document.activeElement !== qaSearchInput) {
      e.preventDefault();
      globalSearchInput?.focus();
    }
  });
}

function setupModals() {
  // Edit Document Modal
  document.getElementById('btn-close-edit-doc')?.addEventListener('click', () => editDocModal.classList.remove('active'));
  document.getElementById('btn-cancel-edit-doc')?.addEventListener('click', () => editDocModal.classList.remove('active'));
  document.getElementById('form-edit-doc')?.addEventListener('submit', async (e) => {
    e.preventDefault();
    const docId = document.getElementById('edit-doc-id').value;
    const body = {
      title: document.getElementById('edit-doc-title').value.trim(),
      subtitle: document.getElementById('edit-doc-subtitle').value.trim(),
      version: document.getElementById('edit-doc-version').value.trim(),
      language: document.getElementById('edit-doc-language').value.trim(),
      primary_project: document.getElementById('edit-doc-project').value.trim(),
      document_type: document.getElementById('edit-doc-type').value.trim(),
      lifecycle_state: document.getElementById('edit-doc-lifecycle').value,
      epistemic_status: document.getElementById('edit-doc-epistemic').value
    };

    try {
      await api.updateDocument(docId, body);
      editDocModal.classList.remove('active');
      await refreshData();
      if (state.get().selectedDocId === docId) {
        renderInspectorView(docId);
      }
    } catch (err) {
      alert('Erro ao atualizar documento: ' + err.message);
    }
  });

  // Relation Modal
  document.getElementById('btn-close-relation')?.addEventListener('click', () => relationModal.classList.remove('active'));
  document.getElementById('btn-cancel-relation')?.addEventListener('click', () => relationModal.classList.remove('active'));
  document.getElementById('form-relation')?.addEventListener('submit', async (e) => {
    e.preventDefault();
    const relId = parseInt(document.getElementById('rel-id').value, 10);
    const body = {
      document_id: document.getElementById('rel-document-id').value.trim(),
      subject: document.getElementById('rel-subject').value.trim(),
      predicate: document.getElementById('rel-predicate').value.trim(),
      object: document.getElementById('rel-object').value.trim()
    };

    try {
      if (relId > 0) {
        await api.updateRelation(relId, body);
      } else {
        await api.createRelation(body);
      }
      relationModal.classList.remove('active');
      await refreshData();
    } catch (err) {
      alert('Erro ao salvar relação: ' + err.message);
    }
  });

  // Project Modal
  document.getElementById('btn-close-project')?.addEventListener('click', () => projectModal.classList.remove('active'));
  document.getElementById('btn-cancel-project')?.addEventListener('click', () => projectModal.classList.remove('active'));
  document.getElementById('form-project')?.addEventListener('submit', async (e) => {
    e.preventDefault();
    const projId = document.getElementById('proj-id').value.trim();
    const domainsStr = document.getElementById('proj-domains').value.trim();
    const domains = domainsStr ? domainsStr.split(',').map(s => s.trim()).filter(Boolean) : [];

    const body = {
      id: projId,
      name: document.getElementById('proj-name').value.trim(),
      kind: document.getElementById('proj-kind').value.trim(),
      stage: document.getElementById('proj-stage').value,
      research_domains: domains
    };

    try {
      await api.createProject(body);
      projectModal.classList.remove('active');
      await refreshData();
    } catch (err) {
      alert('Erro ao salvar projeto: ' + err.message);
    }
  });

  // Delete Confirm Modal
  document.getElementById('btn-cancel-delete')?.addEventListener('click', () => {
    deleteConfirmModal.classList.remove('active');
    pendingDeleteCallback = null;
  });

  document.getElementById('btn-confirm-delete')?.addEventListener('click', async () => {
    if (pendingDeleteCallback) {
      try {
        await pendingDeleteCallback();
        deleteConfirmModal.classList.remove('active');
        pendingDeleteCallback = null;
        await refreshData();
      } catch (err) {
        alert('Erro ao excluir: ' + err.message);
      }
    }
  });

  [editDocModal, relationModal, projectModal, deleteConfirmModal].forEach(m => {
    m?.addEventListener('click', (e) => {
      if (e.target === m) m.classList.remove('active');
    });
  });
}

function promptDelete(message, onConfirm) {
  document.getElementById('delete-confirm-message').textContent = message;
  pendingDeleteCallback = onConfirm;
  deleteConfirmModal.classList.add('active');
}

// Master Render Function
function render(s) {
  // Update Navigation Active State
  navBtns.forEach(btn => {
    btn.classList.toggle('active', btn.dataset.view === s.currentView);
  });

  // Toggle View Container Visibility
  Object.keys(views).forEach(vKey => {
    if (views[vKey]) {
      views[vKey].classList.toggle('active', vKey === s.currentView);
    }
  });

  // Render Canonical Início Widgets
  renderSemanticGraph('main-semantic-graph', s.relations, s.documents);
  renderProvenanceTimeline('main-timeline-container', s.events);

  // Render Subviews
  if (s.currentView === 'corpus') renderCorpusView(s);
  if (s.currentView === 'search') renderSearchView(s);
  if (s.currentView === 'inspector' && s.selectedDocId) renderInspectorView(s.selectedDocId);
  if (s.currentView === 'projects') {
    renderProjectsView(s);
    setTimeout(() => {
      constellationInstance?.resize();
    }, 50);
  }
  if (s.currentView === 'relations') renderRelationsView(s);
  if (s.currentView === 'schemas') renderSchemasView(s);
  if (s.currentView === 'events') renderEventsView(s);
}

function renderCorpusView(s) {
  const grid = document.getElementById('docs-grid');
  if (!grid) return;

  if (s.documents.length === 0) {
    grid.innerHTML = `<div style="grid-column: 1/-1; padding: 2rem; color: var(--cl-text-muted); text-align: center;">Nenhum documento disponível ou autorizado.</div>`;
    return;
  }

  const visMap = {
    'public': { label: '🌐 PÚBLICO', class: 'cl-vis-public' },
    'internal_embrapa': { label: '🏢 EMBRAPA INTERNO', class: 'cl-vis-internal_embrapa' },
    'team': { label: '👥 EQUIPE', class: 'cl-vis-team' },
    'private': { label: '🔒 PRIVADO', class: 'cl-vis-private' }
  };

  grid.innerHTML = s.documents.map(d => {
    const vis = visMap[d.visibility] || visMap['public'];
    return `
    <div class="cl-exp-card" style="border: 1px solid var(--cl-line);">
      <div>
        <div style="display: flex; justify-content: space-between; align-items: flex-start; margin-bottom: 0.4rem;">
          <div style="display: flex; align-items: center; gap: 0.4rem;">
            <span class="cl-exp-code">${d.id}</span>
            <span class="cl-vis-pill ${vis.class}">${vis.label}</span>
          </div>
          <span class="tag tag-declared">v${d.version}</span>
        </div>
        <h4 style="font-family: var(--font-display); font-size: 1rem; color: var(--cl-text-0);">${d.title}</h4>
        ${d.subtitle ? `<p style="font-size: 0.75rem; color: var(--cl-text-muted); margin-top: 0.2rem;">${d.subtitle}</p>` : ''}
      </div>

      <div style="display: flex; gap: 0.4rem; flex-wrap: wrap; margin: 0.75rem 0;">
        <span class="tag tag-project">${d.primary_project}</span>
        <span class="tag tag-declared">${d.primary_authority}</span>
        <span class="tag ${d.text_analyzed ? 'tag-declared' : 'tag-status'}">
          ${d.text_analyzed ? '● APROFUNDADO' : '○ NÃO APROFUNDADO'}
        </span>
        ${d.owner ? `<span class="tag" style="background:rgba(56,152,255,0.1); color:#3898ff;">👤 ${d.owner}</span>` : ''}
      </div>

      <div style="display: flex; justify-content: space-between; align-items: center; border-top: 1px solid var(--cl-line-subtle); padding-top: 0.5rem;">
        <span style="font-size: 0.7rem; color: var(--cl-text-muted);">${d.date_created || ''}</span>
        <button class="btn btn-secondary btn-sm" onclick="window.contextlabInspect('${d.id}')">Abrir Contexto →</button>
      </div>
    </div>
  `;
  }).join('');
}

function renderSearchView(s) {
  const routingBox = document.getElementById('routing-box');
  const resultsContainer = document.getElementById('search-results-container');
  if (!s.searchResults) {
    if (routingBox) routingBox.style.display = 'none';
    if (resultsContainer) resultsContainer.innerHTML = '<div style="color: var(--cl-text-muted);">Digite uma consulta na barra superior.</div>';
    return;
  }

  if (routingBox) {
    routingBox.style.display = 'flex';
    document.getElementById('routing-badge-container').innerHTML = `
      <span class="cl-exp-badge ${s.searchResults.route === 'TEXT_INDEX_REQUIRED' ? 'cl-badge-deep' : 'cl-badge-ready'}">
        ${s.searchResults.route}
      </span>
    `;
    document.getElementById('routing-reason-text').textContent = s.searchResults.reason;
  }

  if (resultsContainer) {
    resultsContainer.innerHTML = s.searchResults.results.map(r => `
      <div class="cl-exp-card" style="margin-bottom: 0.75rem;">
        <div style="display: flex; justify-content: space-between;">
          <span class="cl-exp-code">${r.document_id}</span>
          <span class="tag tag-project">${r.primary_project}</span>
        </div>
        <h4 style="font-family: var(--font-display); font-size: 1rem; margin-top: 0.25rem;">${r.title}</h4>
        <div style="font-size: 0.78rem; color: var(--cl-green); margin: 0.35rem 0;">${r.explanation}</div>
        ${r.snippet ? `<div class="code-block">${r.snippet}</div>` : ''}
        <div style="margin-top: 0.5rem; text-align: right;">
          <button class="btn btn-secondary btn-sm" onclick="window.contextlabInspect('${r.document_id}')">Inspecionar</button>
        </div>
      </div>
    `).join('');
  }
}

window.contextlabInspect = (docId) => {
  state.setView('inspector', docId);
};

window.openEditDocumentModal = (doc) => {
  document.getElementById('edit-doc-id').value = doc.id;
  document.getElementById('edit-doc-title').value = doc.title || '';
  document.getElementById('edit-doc-subtitle').value = doc.subtitle || '';
  document.getElementById('edit-doc-version').value = doc.version || '0.1.0';
  document.getElementById('edit-doc-language').value = doc.language || 'pt-BR';
  document.getElementById('edit-doc-project').value = doc.primary_project || '';
  document.getElementById('edit-doc-type').value = doc.document_type || 'research_note';
  document.getElementById('edit-doc-lifecycle').value = doc.lifecycle_state || 'draft';
  document.getElementById('edit-doc-epistemic').value = doc.epistemic_status || 'working_hypothesis';
  editDocModal.classList.add('active');
};

async function renderInspectorView(docId) {
  const container = document.getElementById('inspector-content');
  if (!container) return;
  container.innerHTML = `<div style="padding: 2rem; color: var(--cl-text-muted);">Carregando...</div>`;

  try {
    const doc = await api.getDocument(docId);
    container.innerHTML = `
      <div class="inspector-container">
        <div class="inspector-header">
          <div>
            <div style="display: flex; align-items: center; gap: 0.5rem; margin-bottom: 0.35rem;">
              <span class="cl-exp-code" style="font-size: 0.9rem;">${doc.id}</span>
              <span class="tag tag-declared">v${doc.version}</span>
              <span class="tag tag-project">${doc.primary_project}</span>
              <span class="tag ${doc.text_analyzed ? 'tag-declared' : 'tag-status'}">
                ${doc.text_analyzed ? '● TEXTO APROFUNDADO' : '○ METADATA-FIRST'}
              </span>
            </div>
            <h2 style="font-family: var(--font-serif); font-size: 1.4rem; color: var(--cl-text-0);">${doc.title}</h2>
          </div>
          <div style="display: flex; gap: 0.5rem;">
            <button class="btn btn-secondary btn-sm" onclick="state.setView('corpus')">← Voltar</button>
            <button class="btn btn-secondary btn-sm" id="btn-edit-doc-action">✏️ Editar</button>
            <button class="btn btn-rose btn-sm" id="btn-delete-doc-action">🗑️ Excluir</button>
            <button class="btn btn-cyan btn-sm" id="btn-deepen-doc">⚡ Aprofundar</button>
          </div>
        </div>

        <div class="inspector-tabs">
          <button class="inspector-tab active" data-tab="tab-overview">Visão Geral</button>
          <button class="inspector-tab" data-tab="tab-context">Contexto Estruturado</button>
          <button class="inspector-tab" data-tab="tab-provenance">Proveniência</button>
          <button class="inspector-tab" data-tab="tab-text">Texto & Leitura</button>
          <button class="inspector-tab" data-tab="tab-json">JSON Bruto</button>
        </div>

        <div class="inspector-body">
          <div class="tab-pane active" id="tab-overview">
            <table class="kv-table">
              <tr><th>Identificador</th><td><code>${doc.id}</code></td></tr>
              <tr><th>Título</th><td>${doc.title}</td></tr>
              <tr><th>Versão</th><td>${doc.version}</td></tr>
              <tr><th>Projeto</th><td>${doc.primary_project}</td></tr>
              <tr><th>Autoridade</th><td><span class="tag tag-declared">${doc.primary_authority}</span></td></tr>
              <tr><th>Status Epistêmico</th><td><span class="tag tag-status">${doc.epistemic_status}</span></td></tr>
            </table>
          </div>

          <div class="tab-pane" id="tab-context">
            ${(doc.metadata_envelopes || []).map(env => `
              <div style="background: rgba(3, 14, 25, 0.7); padding: 0.75rem; border-radius: var(--cl-radius-sm); margin-bottom: 0.75rem;">
                <div style="display: flex; justify-content: space-between; margin-bottom: 0.4rem;">
                  <strong>${env.schema_id}</strong>
                  <span class="tag tag-declared">${env.authority}</span>
                </div>
                <pre class="code-block">${JSON.stringify(env.payload, null, 2)}</pre>
              </div>
            `).join('')}
          </div>

          <div class="tab-pane" id="tab-provenance">
            <p style="font-size: 0.85rem; color: var(--cl-text-1);">Origem e Linhagem Declarada pelo Autor:</p>
            <pre class="code-block">${JSON.stringify(doc.metadata_envelopes?.[0]?.payload?.provenance || {}, null, 2)}</pre>
          </div>

          <div class="tab-pane" id="tab-text">
            ${doc.text_analysis ? `
              <div style="display: grid; grid-template-columns: repeat(4, 1fr); gap: 0.75rem; margin-bottom: 1rem;">
                <div class="cl-exp-card"><span class="cl-exp-code">Caracteres</span><h3 style="color:var(--cl-cyan);">${doc.text_analysis.character_count}</h3></div>
                <div class="cl-exp-card"><span class="cl-exp-code">Palavras</span><h3 style="color:var(--cl-green);">${doc.text_analysis.word_count}</h3></div>
                <div class="cl-exp-card"><span class="cl-exp-code">Linhas</span><h3 style="color:var(--cl-gold);">${doc.text_analysis.line_count}</h3></div>
                <div class="cl-exp-card"><span class="cl-exp-code">Seções</span><h3 style="color:var(--cl-blue);">${doc.text_analysis.section_count}</h3></div>
              </div>
              <div class="code-block" style="max-height: 250px;">${doc.text_analysis.sample_preview}</div>
            ` : `<p style="color: var(--cl-text-muted);">Texto ainda não aprofundado.</p>`}
          </div>

          <div class="tab-pane" id="tab-json">
            <pre class="code-block">${JSON.stringify(doc, null, 2)}</pre>
          </div>
        </div>
      </div>
    `;

    // Action buttons
    container.querySelector('#btn-edit-doc-action')?.addEventListener('click', () => window.openEditDocumentModal(doc));
    container.querySelector('#btn-delete-doc-action')?.addEventListener('click', () => {
      promptDelete(`Excluir documento ${doc.id}?`, async () => {
        await api.deleteDocument(doc.id);
        state.setView('corpus');
      });
    });

    container.querySelector('#btn-deepen-doc')?.addEventListener('click', async () => {
      await api.deepen(doc.id);
      renderInspectorView(doc.id);
    });

    // Tab switching
    const tabs = container.querySelectorAll('.inspector-tab');
    const panes = container.querySelectorAll('.tab-pane');
    tabs.forEach(tab => {
      tab.addEventListener('click', () => {
        tabs.forEach(t => t.classList.remove('active'));
        panes.forEach(p => p.classList.remove('active'));
        tab.classList.add('active');
        container.querySelector(`#${tab.dataset.tab}`)?.classList.add('active');
      });
    });

  } catch (err) {
    container.innerHTML = `<div style="color: var(--cl-rose);">Erro: ${err.message}</div>`;
  }
}

function renderProjectsView(s) {
  const container = document.getElementById('projects-content');
  if (!container) return;
  container.innerHTML = `
    <div style="background: var(--cl-panel-0); border: 1px solid var(--cl-line); border-radius: var(--cl-radius-lg); padding: 1.5rem;">
      <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.25rem;">
        <h2 style="font-family: var(--font-display); font-size: 1.3rem;">Projetos Registrados (${s.projects.length})</h2>
        <button class="btn btn-primary btn-sm" onclick="document.getElementById('project-modal').classList.add('active')">+ Novo Projeto</button>
      </div>
      <div style="display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 1rem;">
        ${s.projects.map(p => `
          <div class="cl-exp-card">
            <div style="display: flex; justify-content: space-between;">
              <span class="cl-exp-code">${p.id}</span>
              <span class="tag tag-status">${p.stage}</span>
            </div>
            <h4 style="font-family: var(--font-display); font-size: 1.05rem; margin-top: 0.35rem;">${p.name}</h4>
            <div style="font-size: 0.75rem; color: var(--cl-cyan); margin-top: 0.25rem;">${p.kind}</div>
          </div>
        `).join('')}
      </div>
    </div>
  `;
}

function renderRelationsView(s) {
  const container = document.getElementById('relations-content');
  if (!container) return;
  container.innerHTML = `
    <div style="background: var(--cl-panel-0); border: 1px solid var(--cl-line); border-radius: var(--cl-radius-lg); padding: 1.5rem;">
      <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 1.25rem;">
        <h2 style="font-family: var(--font-display); font-size: 1.3rem;">Relações Triádicas no Corpus (${s.relations.length})</h2>
        <button class="btn btn-primary btn-sm" onclick="document.getElementById('relation-modal').classList.add('active')">+ Nova Relação</button>
      </div>
      <table class="kv-table">
        <thead>
          <tr><th>Documento</th><th>Sujeito</th><th>Predicado</th><th>Objeto</th></tr>
        </thead>
        <tbody>
          ${s.relations.map(r => `
            <tr>
              <td><code>${r.document_id || '-'}</code></td>
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
  if (!container) return;
  container.innerHTML = `
    <div style="background: var(--cl-panel-0); border: 1px solid var(--cl-line); border-radius: var(--cl-radius-lg); padding: 1.5rem;">
      <h2 style="font-family: var(--font-display); font-size: 1.3rem; margin-bottom: 1rem;">Schema Registry (${s.schemas.length})</h2>
      ${s.schemas.map(sc => `
        <div style="background: rgba(3, 14, 25, 0.7); padding: 1rem; border-radius: var(--cl-radius-sm); margin-bottom: 1rem;">
          <div style="display: flex; justify-content: space-between; margin-bottom: 0.5rem;">
            <strong>${sc.id}</strong>
            <span class="tag tag-declared">v${sc.version}</span>
          </div>
          <pre class="code-block">${JSON.stringify(sc.schema_json, null, 2)}</pre>
        </div>
      `).join('')}
    </div>
  `;
}

function renderEventsView(s) {
  const container = document.getElementById('events-content');
  if (!container) return;
  container.innerHTML = `
    <div style="background: var(--cl-panel-0); border: 1px solid var(--cl-line); border-radius: var(--cl-radius-lg); padding: 1.5rem;">
      <h2 style="font-family: var(--font-display); font-size: 1.3rem; margin-bottom: 1rem;">Trilha de Auditoria & Ingestões Recentes</h2>
      <table class="kv-table">
        <thead>
          <tr><th>Timestamp</th><th>Arquivo</th><th>Document ID</th><th>Status</th></tr>
        </thead>
        <tbody>
          ${s.events.map(ev => `
            <tr>
              <td>${ev.timestamp}</td>
              <td><strong>${ev.source_file}</strong></td>
              <td><code>${ev.document_id || '-'}</code></td>
              <td><span class="tag tag-declared">${ev.status}</span></td>
            </tr>
          `).join('')}
        </tbody>
      </table>
    </div>
  `;
}

// Start Application on Load
document.addEventListener('DOMContentLoaded', initApp);
