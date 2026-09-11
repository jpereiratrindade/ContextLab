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

  const currentDocs = state?.get()?.documents || [];
  let targetDoc = null;

  if (docId) {
    targetDoc = currentDocs.find(d => d.id.toLowerCase() === docId.toLowerCase());
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
      console.warn('Could not fetch full doc details, using local data:', e);
    }
  }

  const qLower = question.toLowerCase();
  const isDeepeningRequired = qLower.includes('misturar') || 
                              qLower.includes('por que') ||
                              qLower.includes('consequências') ||
                              qLower.includes('riscos') ||
                              qLower.includes('como funciona') ||
                              qLower.includes('aprofundamento');

  // ALWAYS render both cards with complementary rich information
  renderMetadataFirstCard(metaCard, question, fullDoc, isDeepeningRequired);
  renderDeepeningCard(deepCard, question, fullDoc, state, isDeepeningRequired);

  if (isDeepeningRequired) {
    if (metaCard) {
      metaCard.style.opacity = '0.75';
      metaCard.classList.remove('active-answer');
    }
    if (deepCard) {
      deepCard.style.opacity = '1';
      deepCard.classList.add('active-answer');
      deepCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  } else {
    if (deepCard) {
      deepCard.style.opacity = '0.75';
      deepCard.classList.remove('active-answer');
    }
    if (metaCard) {
      metaCard.style.opacity = '1';
      metaCard.classList.add('active-answer');
      metaCard.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
    }
  }
}

function renderMetadataFirstCard(container, question, doc, isDeepeningRequired) {
  if (!container) return;

  const title = doc?.title || 'ContextLab — Laboratório de contexto documental computável';
  const project = doc?.primary_project || 'ContextLab';
  const docType = doc?.document_type || 'Artigo técnico';
  const date = doc?.date_created || '2024-12-07';
  const source = doc?.id ? `${doc.id}.md` : 'artigo-exemplo.pdf';
  const authority = doc?.primary_authority || 'DECLARED';
  const owner = doc?.owner ? doc.owner : 'Equipe ContextLab';

  let answerSummary = `O projeto é o <strong>${project}</strong>, um laboratório de contexto documental computável, focado em pesquisa independente sobre metadados, proveniência e aprofundamento seletivo.`;
  const qLower = question.toLowerCase();

  if (qLower.includes('autor') || qLower.includes('quem')) {
    answerSummary = `Os autores declarados são <strong>${owner}</strong> com autoridade primária <strong>${authority}</strong>.`;
  } else if (qLower.includes('relacionam') || qLower.includes('relação')) {
    answerSummary = `O documento relaciona-se com o projeto <strong>${project}</strong> e com o ecossistema de infraestrutura de pesquisa documental computável.`;
  } else if (isDeepeningRequired) {
    answerSummary = `Os metadados registram o enquadramento do documento <code>${doc?.id || 'CONTEXTLAB-BOOTSTRAP-001'}</code>, mas a explicação causal exige análise de conteúdo no card ao lado.`;
  }

  const badgeText = isDeepeningRequired ? 'Metadados de Enquadramento' : 'Respondida por Metadados';
  const badgeClass = isDeepeningRequired ? 'cl-badge-ready' : 'cl-badge-ready';

  container.innerHTML = `
    <div>
      <div class="cl-answer-meta-header">
        <div class="cl-answer-title">Resposta por Metadados</div>
        <span class="cl-exp-badge ${badgeClass}">${badgeText}</span>
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
      ✓ Consulta resolvida a partir de metadados declarados e proveniência auditável.
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
          Esta pergunta requer leitura do conteúdo textual para explicar as distinções epistemológicas entre dados declarados pelo autor e dados derivados computacionalmente. O ContextLab isola apenas os trechos pertinentes sem ler o documento inteiro.
        </div>

        <button class="cl-btn-deepen-primary" id="btn-trigger-deepen-flow" style="cursor: pointer;">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z"/><path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z"/></svg>
          ${isAnalyzed ? 'Ver Leitura Seletiva & Evidências' : 'Aprofundar com Leitura Seletiva'}
        </button>

        <div style="font-size: 0.72rem; color: var(--cl-text-muted); margin: 0.6rem 0 0.4rem 0;">
          Rastreabilidade granular e evidências textuais delimitadas:
        </div>

        <ul class="cl-deepen-checklist">
          <li><span class="cl-deepen-check">✓</span> Leitura seletiva delimitada (economia computacional e foco)</li>
          <li><span class="cl-deepen-check">✓</span> Trechos com offset e hash de integridade</li>
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
          Os metadados foram suficientes para responder a esta consulta. Se você desejar extrair citações textuais completas ou verificar a integridade parágrafo a parágrafo, execute a leitura seletiva.
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

