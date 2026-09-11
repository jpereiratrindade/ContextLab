// ==========================================================================
// ContextLab — Laboratory Experiments Handler (WEB-EXPERIENCE-001)
// ==========================================================================

import { handleQuestionQuery } from './qa_engine.js';

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
    const docId = 'CONTEXTLAB-BOOTSTRAP-001';
    await handleQuestionQuery('Qual é o projeto?', docId, state);
  });

  // CL-EXP-001: RES-SAIT
  const btn001 = document.getElementById('btn-test-exp-001');
  btn001?.addEventListener('click', async (e) => {
    e.preventDefault();
    selectExpCard(btn001.closest('.cl-exp-card'));
    const docId = 'RES-SAIT-NOTE-001';
    await handleQuestionQuery('Qual é o projeto?', docId, state);
  });

  // CL-EXP-002: Aprofundamento Seletivo
  const btn002 = document.getElementById('btn-test-exp-002');
  btn002?.addEventListener('click', async (e) => {
    e.preventDefault();
    selectExpCard(btn002.closest('.cl-exp-card'));
    const docId = 'CONTEXTLAB-BOOTSTRAP-001';
    await handleQuestionQuery('Por que não devemos misturar metadados declarados e derivados?', docId, state);
  });
}

