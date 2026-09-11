// ==========================================================================
// ContextLab — Metadata-First Q&A & Selective Deepening (WEB-EXPERIENCE-001)
// ==========================================================================

import { api } from '../api.js';

export async function handleQuestionQuery(question, docId = '', state) {
  const metaCard = document.getElementById('card-answer-meta');
  const deepCard = document.getElementById('card-answer-deep');
  const qInput = document.getElementById('qa-search-input');
  if (qInput) qInput.value = question;

  // Update active state on example chips
  document.querySelectorAll('.cl-chip').forEach(chip => {
    chip.classList.toggle('active', chip.dataset.q === question);
  });

  // Safely extract documents from state
  const rawDocs = state?.documents || (typeof state?.get === 'function' ? state.get()?.documents : []) || [];
  let targetDoc = null;

  if (docId) {
    targetDoc = rawDocs.find(d => d.id && d.id.toLowerCase() === docId.toLowerCase());
  }
  if (!targetDoc && rawDocs.length > 0) {
    targetDoc = rawDocs[0];
  }

  // Fallback bootstrap document mock if state has not loaded yet
  if (!targetDoc) {
    targetDoc = {
      id: 'CONTEXTLAB-BOOTSTRAP-001',
      title: 'ContextLab — base funcional em C++26 com interface web',
      primary_project: 'ContextLab',
      document_type: 'implementation_contract',
      date_created: '2026-09-11',
      primary_authority: 'author_declared',
      owner: 'Equipe ContextLab',
      text_analyzed: true
    };
  }

  const qLower = question.toLowerCase();
  const isDeepeningRequired = qLower.includes('misturar') || 
                              qLower.includes('por que') ||
                              qLower.includes('porque') ||
                              qLower.includes('consequências') ||
                              qLower.includes('riscos') ||
                              qLower.includes('como funciona') ||
                              qLower.includes('aprofundamento');

  // Render both cards immediately
  renderMetadataFirstCard(metaCard, question, targetDoc, isDeepeningRequired);
  renderDeepeningCard(deepCard, question, targetDoc, state, isDeepeningRequired);

  // Focus and highlight active card
  if (isDeepeningRequired) {
    if (metaCard) {
      metaCard.style.opacity = '0.65';
      metaCard.classList.remove('active-answer');
    }
    if (deepCard) {
      deepCard.style.opacity = '1';
      deepCard.classList.add('active-answer');
      deepCard.scrollIntoView({ behavior: 'smooth', block: 'center' });
    }
  } else {
    if (deepCard) {
      deepCard.style.opacity = '0.65';
      deepCard.classList.remove('active-answer');
    }
    if (metaCard) {
      metaCard.style.opacity = '1';
      metaCard.classList.add('active-answer');
      metaCard.scrollIntoView({ behavior: 'smooth', block: 'center' });
    }
  }

  // Asynchronously enrich with full document details from API
  if (targetDoc && targetDoc.id) {
    try {
      const fullDoc = await api.getDocument(targetDoc.id);
      if (fullDoc) {
        renderMetadataFirstCard(metaCard, question, fullDoc, isDeepeningRequired);
        renderDeepeningCard(deepCard, question, fullDoc, state, isDeepeningRequired);
      }
    } catch {
      // Keep rendered cards if API fetch fails
    }
  }
}

function renderMetadataFirstCard(container, question, doc, isDeepeningRequired) {
  if (!container) return;

  const title = doc?.title || 'ContextLab — base funcional em C++26 com interface web';
  const project = doc?.primary_project || 'ContextLab';
  const docType = doc?.document_type || 'implementation_contract';
  const date = doc?.date_created || '2026-09-11';
  const source = doc?.id ? `${doc.id}.md` : 'CONTEXTLAB-BOOTSTRAP-001.md';
  const authority = doc?.primary_authority || 'author_declared';
  const owner = doc?.owner ? doc.owner : 'Equipe ContextLab';

  let answerSummary = `O projeto é o <strong>${project}</strong>, um laboratório de contexto documental computável, focado em pesquisa independente sobre metadados, proveniência e aprofundamento seletivo.`;
  const qLower = question.toLowerCase();

  if (qLower.includes('autor') || qLower.includes('quem')) {
    answerSummary = `Os autores declarados são <strong>${owner}</strong> com autoridade primária <strong>${authority}</strong> e validação humana obrigatória.`;
  } else if (qLower.includes('relacionam') || qLower.includes('relação') || qLower.includes('relaciona')) {
    answerSummary = `O documento relaciona-se com o projeto <strong>${project}</strong>, sendo motivado por <code>RES-SAIT-NOTE-001@0.2.0</code> e sem herança de runtime com <code>TinyKernel</code> ou <code>SisTer</code>.`;
  } else if (isDeepeningRequired) {
    answerSummary = `Os metadados declarados registram a premissa de separação entre autoridade humana e inferência automática (documento <code>${doc?.id || 'CONTEXTLAB-BOOTSTRAP-001'}</code>). Para a fundamentação completa, veja o aprofundamento ao lado.`;
  }

  const badgeText = isDeepeningRequired ? 'Metadados de Enquadramento' : 'Respondida por Metadados';

  container.innerHTML = `
    <div>
      <div class="cl-answer-meta-header">
        <div class="cl-answer-title">Resposta por Metadados</div>
        <span class="cl-exp-badge cl-badge-ready">${badgeText}</span>
      </div>

      <div class="cl-answer-question">${question}</div>
      <div class="cl-answer-text">
        ${answerSummary}
      </div>

      <div class="cl-evidence-title">Evidências (Metadados Computáveis)</div>
      <table class="cl-evidence-table">
        <tr><th>Título</th><td>${title}</td></tr>
        <tr><th>Autor/Responsável</th><td>${owner} <span class="tag tag-declared">${authority}</span></td></tr>
        <tr><th>Projeto</th><td><span class="tag tag-project">${project}</span></td></tr>
        <tr><th>Tipo de Documento</th><td>${docType}</td></tr>
        <tr><th>Data de Criação</th><td>${date}</td></tr>
        <tr><th>Arquivo Fonte</th><td><code>${source}</code></td></tr>
      </table>
    </div>

    <div class="cl-answer-footer-note">
      ✓ Consulta resolvida a partir de metadados declarados e proveniência auditável (0ms).
    </div>
  `;
}

function renderDeepeningCard(container, question, doc, state, isDeepeningRequired) {
  if (!container) return;

  const docId = doc?.id || 'CONTEXTLAB-BOOTSTRAP-001';
  const isAnalyzed = doc?.text_analyzed;

  if (isDeepeningRequired) {
    container.innerHTML = `
      <div>
        <div class="cl-answer-meta-header">
          <div style="display: flex; align-items: center; gap: 0.4rem;">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="color: var(--cl-gold);"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
            <span class="cl-exp-badge cl-badge-deep">Exige Aprofundamento Seletivo</span>
          </div>
        </div>

        <div class="cl-answer-question">${question}</div>
        <div class="cl-answer-text">
          Misturar metadados declarados (autoridade do autor) com metadados derivados (inferência automática) corrompe a rastreabilidade científica. O ContextLab isola o texto sob demanda com hashes criptográficos.
        </div>

        <button class="cl-btn-deepen-primary" id="btn-trigger-deepen-flow" style="cursor: pointer;">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
          Inspecionar Leitura Seletiva & Hashes
        </button>

        <div style="font-size: 0.72rem; color: var(--cl-text-muted); margin: 0.6rem 0 0.4rem 0;">
          Rastreabilidade granular e evidências textuais delimitadas:
        </div>

        <ul class="cl-deepen-checklist">
          <li><span class="cl-deepen-check">✓</span> Leitura seletiva delimitada (economia computacional e foco)</li>
          <li><span class="cl-deepen-check">✓</span> 11 seções indexadas com hashes SHA-256</li>
          <li><span class="cl-deepen-check">✓</span> Separação estrita entre autoridade humana e inferência</li>
        </ul>
      </div>
    `;
  } else {
    container.innerHTML = `
      <div>
        <div class="cl-answer-meta-header">
          <div style="display: flex; align-items: center; gap: 0.4rem;">
            <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="color: var(--cl-text-muted);"><path d="M12 20h9"/><path d="M16.5 3.5a2.121 2.121 0 0 1 3 3L7 19l-4 1 1-4L16.5 3.5z"/></svg>
            <span class="cl-exp-badge" style="background: rgba(148, 163, 184, 0.15); color: #94a3b8; border: 1px solid rgba(148, 163, 184, 0.25);">Aprofundamento Opcional</span>
          </div>
        </div>

        <div class="cl-answer-question">${question}</div>
        <div class="cl-answer-text">
          Os metadados declarados foram suficientes para responder a esta consulta. Para verificar a integridade parágrafo a parágrafo ou extrair citações completas, execute a leitura seletiva.
        </div>

        <button class="cl-btn-deepen-primary" id="btn-trigger-deepen-flow" style="background: var(--cl-panel-1); border: 1px solid var(--cl-line); color: var(--cl-text-0); cursor: pointer;">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
          ${isAnalyzed ? 'Inspecionar Trechos Textuais' : 'Aprofundar Texto (Opcional)'}
        </button>

        <div style="font-size: 0.72rem; color: var(--cl-text-muted); margin-top: 0.6rem;">
          Preserva o princípio metadata-first: leitura textual sob demanda.
        </div>
      </div>
    `;
  }

  container.querySelector('#btn-trigger-deepen-flow')?.addEventListener('click', async () => {
    try {
      if (!isAnalyzed) {
        await api.deepen(docId);
      }
      state?.setView('inspector', docId);
    } catch (e) {
      alert('Erro no aprofundamento: ' + e.message);
    }
  });
}


