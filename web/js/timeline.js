// ==========================================================================
// ContextLab — Provenance Timeline Component (WEB-EXPERIENCE-001)
// ==========================================================================

export function renderProvenanceTimeline(containerId, events = []) {
  const container = document.getElementById(containerId);
  if (!container) return;

  if (events && events.length > 0) {
    const displayEvents = events.slice(0, 4);
    container.innerHTML = displayEvents.map(ev => {
      const timeStr = ev.timestamp ? ev.timestamp.substring(0, 16) : 'Recente';
      let title = 'Documento Ingerido';
      if (ev.event === 'DOCUMENT_INGESTED') title = 'Documento ingerido';
      else if (ev.event === 'TEXT_DEEPENED') title = 'Aprofundamento Textual';
      else if (ev.event === 'SCHEMA_REGISTERED') title = 'Schema Registrado';

      return `
        <div class="cl-timeline-item">
          <div class="cl-timeline-dot"></div>
          <div class="cl-timeline-time">${timeStr}</div>
          <div class="cl-timeline-event">${title}</div>
          <div class="cl-timeline-detail">${ev.source_file || ev.document_id || ev.message || ''}</div>
        </div>
      `;
    }).join('');
  } else {
    // Canonical default timeline
    container.innerHTML = `
      <div class="cl-timeline-item">
        <div class="cl-timeline-dot"></div>
        <div class="cl-timeline-time">2024-12-07 14:23</div>
        <div class="cl-timeline-event">Documento ingerido</div>
        <div class="cl-timeline-detail">artigo-exemplo.pdf</div>
      </div>
      <div class="cl-timeline-item">
        <div class="cl-timeline-dot"></div>
        <div class="cl-timeline-time">2024-12-07 14:23</div>
        <div class="cl-timeline-event">Metadados extraídos</div>
        <div class="cl-timeline-detail">12 campos · SHA-256: a349...7c2e</div>
      </div>
      <div class="cl-timeline-item">
        <div class="cl-timeline-dot"></div>
        <div class="cl-timeline-time">2024-12-07 14:24</div>
        <div class="cl-timeline-event">Relacionamentos identificados</div>
        <div class="cl-timeline-detail">3 documentos · 5 conceitos</div>
      </div>
      <div class="cl-timeline-item">
        <div class="cl-timeline-dot"></div>
        <div class="cl-timeline-time">2024-12-07 14:25</div>
        <div class="cl-timeline-event">Indexado no SQLite/FTS5</div>
        <div class="cl-timeline-detail">pronto para busca</div>
      </div>
    `;
  }
}
