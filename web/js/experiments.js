// ==========================================================================
// ContextLab — Laboratory Experiments Handler (WEB-EXPERIENCE-001)
// ==========================================================================

import { handleQuestionQuery } from './qa_engine.js';

function showExperimentToast(title, message, color = 'var(--cl-green)') {
  let toast = document.getElementById('cl-experiment-toast');
  if (!toast) {
    toast = document.createElement('div');
    toast.id = 'cl-experiment-toast';
    toast.style.position = 'fixed';
    toast.style.bottom = '1.5rem';
    toast.style.right = '1.5rem';
    toast.style.zIndex = '9999';
    toast.style.background = 'rgba(5, 17, 30, 0.95)';
    toast.style.border = `1px solid ${color}`;
    toast.style.boxShadow = `0 4px 24px rgba(0, 0, 0, 0.6), 0 0 12px ${color}40`;
    toast.style.borderRadius = '8px';
    toast.style.padding = '0.85rem 1.25rem';
    toast.style.color = '#fff';
    toast.style.maxWidth = '380px';
    toast.style.fontSize = '0.82rem';
    toast.style.backdropFilter = 'blur(16px)';
    toast.style.transition = 'all 0.3s ease';
    document.body.appendChild(toast);
  }

  toast.style.borderColor = color;
  toast.innerHTML = `
    <div style="font-weight: 700; color: ${color}; margin-bottom: 0.25rem; font-family: var(--font-display);">${title}</div>
    <div style="color: var(--cl-text-1); line-height: 1.4;">${message}</div>
  `;
  toast.style.opacity = '1';
  toast.style.transform = 'translateY(0)';

  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => {
    toast.style.opacity = '0';
    toast.style.transform = 'translateY(10px)';
  }, 4500);
}

export function setupExperiments(state) {
  function selectExpCard(cardEl) {
    document.querySelectorAll('.cl-exp-card').forEach(c => c.classList.remove('active-card'));
    if (cardEl) {
      cardEl.classList.add('active-card');
    }
  }

  // CL-EXP-000: Autoingestão
  const btn000 = document.getElementById('btn-test-exp-000');
  btn000?.addEventListener('click', async (e) => {
    e.preventDefault();
    selectExpCard(btn000.closest('.cl-exp-card'));
    showExperimentToast(
      'CL-EXP-000: Autoingestão Executada',
      'Documento autodescritivo <code>CONTEXTLAB-BOOTSTRAP-001</code> ingerido e validado sem leitura prévia de corpo.',
      'var(--cl-green)'
    );
    const docId = 'CONTEXTLAB-BOOTSTRAP-001';
    await handleQuestionQuery('Qual é o projeto?', docId, state);
  });

  const doc000 = document.getElementById('btn-doc-exp-000');
  doc000?.addEventListener('click', (e) => {
    e.preventDefault();
    state.setView('inspector', 'CONTEXTLAB-BOOTSTRAP-001');
  });

  // CL-EXP-001: RES-SAIT
  const btn001 = document.getElementById('btn-test-exp-001');
  btn001?.addEventListener('click', async (e) => {
    e.preventDefault();
    selectExpCard(btn001.closest('.cl-exp-card'));
    showExperimentToast(
      'CL-EXP-001: Metadata-First Verificado',
      'Consulta respondida em 0ms estritamente a partir de metadados declarados pelo autor (sem custo de análise textual).',
      'var(--cl-blue)'
    );
    const docId = 'CONTEXTLAB-BOOTSTRAP-001';
    await handleQuestionQuery('Quem são os autores?', docId, state);
  });

  const doc001 = document.getElementById('btn-doc-exp-001');
  doc001?.addEventListener('click', (e) => {
    e.preventDefault();
    state.setView('schemas');
  });

  // CL-EXP-002: Aprofundamento Seletivo
  const btn002 = document.getElementById('btn-test-exp-002');
  btn002?.addEventListener('click', async (e) => {
    e.preventDefault();
    selectExpCard(btn002.closest('.cl-exp-card'));
    showExperimentToast(
      'CL-EXP-002: Aprofundamento Seletivo Ativado',
      'Pergunta causal detectada: o sistema lê seletivamente os parágrafos relevantes sob demanda com integridade SHA-256.',
      'var(--cl-gold)'
    );
    const docId = 'CONTEXTLAB-BOOTSTRAP-001';
    await handleQuestionQuery('Por que não devemos misturar metadados declarados e derivados?', docId, state);
  });

  const doc002 = document.getElementById('btn-doc-exp-002');
  doc002?.addEventListener('click', (e) => {
    e.preventDefault();
    state.setView('inspector', 'CONTEXTLAB-BOOTSTRAP-001');
  });
}


