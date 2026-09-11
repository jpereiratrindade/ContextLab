/**
 * ContextLab API Client module
 */

const BASE_URL = '/api/v1';

export const api = {
  async getHealth() {
    const res = await fetch(`${BASE_URL}/health`);
    return res.json();
  },

  async getSystemInfo() {
    const res = await fetch(`${BASE_URL}/system`);
    return res.json();
  },

  async listDocuments() {
    const res = await fetch(`${BASE_URL}/documents`);
    return res.json();
  },

  async getDocument(id) {
    const res = await fetch(`${BASE_URL}/documents/${encodeURIComponent(id)}`);
    if (!res.ok) throw new Error(`Document ${id} not found`);
    return res.json();
  },

  async listProjects() {
    const res = await fetch(`${BASE_URL}/projects`);
    return res.json();
  },

  async listRelations() {
    const res = await fetch(`${BASE_URL}/relations`);
    return res.json();
  },

  async listSchemas() {
    const res = await fetch(`${BASE_URL}/schemas`);
    return res.json();
  },

  async getRecentEvents() {
    const res = await fetch(`${BASE_URL}/events`);
    return res.json();
  },

  async search(query, mode = 'auto') {
    const res = await fetch(`${BASE_URL}/search`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ query, mode })
    });
    return res.json();
  },

  async deepen(id) {
    const res = await fetch(`${BASE_URL}/documents/${encodeURIComponent(id)}/deepen`, {
      method: 'POST'
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao aprofundar texto');
    }
    return res.json();
  },

  async uploadFile(file) {
    const formData = new FormData();
    formData.append('file', file);
    const res = await fetch(`${BASE_URL}/ingest`, {
      method: 'POST',
      body: formData
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha na ingestão');
    }
    return res.json();
  }
};
