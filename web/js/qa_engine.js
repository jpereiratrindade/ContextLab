// ==========================================================================
// ContextLab — Metadata-First Q&A & Selective Deepening (WEB-EXPERIENCE-001)
// ==========================================================================

import { api } from '../api.js';

export async function handleQuestionQuery(question, docId = '', state) {
  const metaCard = document.getElementById('card-answer-meta');
  const deepCard = document.getElementById('card-answer-deep');
  const qInput = document.getElementById('qa-search-input');
  if (qInput) qInput.value = question;

  const currentDocs = state.get().documents || [];
  let targetDoc = null;

  if (docId) {
    targetDoc = currentDocs.find(d => d.id === docId);
  }
  if (!targetDoc && currentDocs.length > 0) {
    targetDoc = currentDocs[0];
  }

  // Fetch full details if available
  let fullDoc = targetDoc;
  if (targetDoc && targetDoc.id) {
    try {
      fullDoc = await api.getDocument(targetDoc.id);
    } catch (e) {
      console.warn('Could not fetch full doc:', e);
    }
  }

  const isDeepeningRequired = question.toLowerCase().includes('misturar') || 
                              question.toLowerCase().includes('por que') ||
                              question.toLowerCase().includes('consequências') ||
                              question.toLowerCase().includes('riscos');

  if (isDeepeningRequired) {
    // Focus Amber Deepening Card
    renderDeepeningCard(deepCard, question, fullDoc, state);
    if (metaCard) metaCard.style.opacity = '0.45';
    if (deepCard) {
      deepCard.style.opacity = '1';
      deepCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  } else {
    // Focus Green Metadata-First Card
    renderMetadataFirstCard(metaCard, question, fullDoc);
    if (deepCard) deepCard.style.opacity = '0.45';
    if (metaCard) {
      metaCard.style.opacity = '1';
      metaCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  }
}

function renderMetadataFirstCard(container, question, doc) {
  if (!container) return;

  const title = doc?.title || 'ContextLab — Laboratório de contexto documental computável';
  const project = doc?.primary_project || 'ContextLab';
  const docType = doc?.document_type || 'Artigo técnico';
  const date = doc?.date_created || '2024-12-07';
  const source = doc?.id ? `${doc.id}.md` : 'artigo-exemplo.pdf';
  const authority = doc?.primary_authority || 'DECLARED';

  container.innerHTML = `
    <div>
      <div class="cl-answer-meta-header">
        <div class="cl-answer-title">Resposta</div>
        <span class="cl-exp-badge cl-badge-ready">Respondida por metadados</span>
      </div>

      <div class="cl-answer-question">${question}</div>
      <div class="cl-answer-text">
        O projeto é o <strong>${project}</strong>, um laboratório de contexto documental computável, focado em pesquisa independente sobre metadados, proveniência e aprofundamento seletivo.
      </div>

      <div class="cl-evidence-title">Evidências (metadados)</div>
      <table class="cl-evidence-table">
        <tr><th>Título</th><td>${title}</td></tr>
        <tr><th>Autor</th><td>Equipe ContextLab <span class="tag tag-declared">${authority}</span></td></tr>
        <tr><th>Tipo</th><td>${docType}</td></tr>
        <tr><th>Assunto</th><td>contexto documental; metadados; proveniência</td></tr>
        <tr><th>Data</th><td>${date}</td></tr>
        <tr><th>Fonte</th><td><code>${source}</code></td></tr>
      </table>
    </div>

    <div class="cl-answer-footer-note">
      Resposta gerada exclusivamente a partir de metadados declarados e/ou derivados.
    </div>
  `;
}

function renderDeepeningCard(container, question, doc, state) {
  if (!container) return;

  const docId = doc?.id || 'CONTEXTLAB-BOOTSTRAP-001';
  const isAnalyzed = doc?.text_analyzed;

  container.innerHTML = `
    <div>
      <div class="cl-answer-meta-header">
        <div style="display: flex; align-items: center; gap: 0.4rem;">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="color: var(--cl-gold);"><circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/></svg>
          <span class="cl-exp-badge cl-badge-deep">Exige aprofundamento</span>
        </div>
      </div>

      <div class="cl-answer-question">${question}</div>
      <div class="cl-answer-text">
        Essa pergunta requer análise do conteúdo textual para explicar os riscos e as consequências epistemológicas. O ContextLab seleciona apenas os trechos relevantes no documento.
      </div>

      <button class="cl-btn-deepen-primary" id="btn-trigger-deepen-flow">
        <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
        ${isAnalyzed ? 'Ver Leitura Seletiva & Evidências' : 'Aprofundar com leitura seletiva'}
      </button>

      <div style="font-size: 0.7rem; color: var(--cl-text-muted); margin-bottom: 0.5rem;">
        Serão analisados apenas os trechos relevantes, com rastreabilidade e evidências.
      </div>

      <ul class="cl-deepen-checklist">
        <li><span class="cl-deepen-check">✓</span> Leitura seletiva (não lê o documento inteiro)</li>
        <li><span class="cl-deepen-check">✓</span> Trechos com evidência e localização</li>
        <li><span class="cl-deepen-check">✓</span> Resposta com referências e contexto</li>
      </ul>
    </div>
  `;

  container.querySelector('#btn-trigger-deepen-flow')?.addEventListener('click', async () => {
    try {
      if (!isAnalyzed) {
        await api.deepen(docId);
      }
      state.setView('inspector', docId);
    } catch (e) {
      alert('Erro no aprofundamento: ' + e.message);
    }
  });
}
