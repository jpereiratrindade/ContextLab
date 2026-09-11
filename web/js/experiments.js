// ==========================================================================
// ContextLab — Laboratory Experiments Handler (WEB-EXPERIENCE-001)
// ==========================================================================

import { api } from '../api.js';
import { handleQuestionQuery } from './qa_engine.js';

export function setupExperiments(state) {
  // CL-EXP-000: Autoingestão
  document.getElementById('btn-test-exp-000')?.addEventListener('click', async () => {
    const docId = 'CONTEXTLAB-BOOTSTRAP-001';
    await handleQuestionQuery('Qual é o projeto?', docId, state);
  });

  // CL-EXP-001: RES-SAIT
  document.getElementById('btn-test-exp-001')?.addEventListener('click', async () => {
    const docId = 'RES-SAIT-NOTE-001';
    await handleQuestionQuery('Qual é o projeto?', docId, state);
  });

  // CL-EXP-002: Aprofundamento Seletivo
  document.getElementById('btn-test-exp-002')?.addEventListener('click', async () => {
    const docId = 'CONTEXTLAB-BOOTSTRAP-001';
    await handleQuestionQuery('Por que não devemos misturar metadados declarados e derivados?', docId, state);
  });
}
