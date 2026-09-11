/**
 * ContextLab API Client module
 */

const BASE_URL = '/api/v1';
const TOKEN_STORAGE_KEY = 'contextlab_auth_token';

export const api = {
  getToken() {
    return localStorage.getItem(TOKEN_STORAGE_KEY) || '';
  },

  setToken(token) {
    if (token) localStorage.setItem(TOKEN_STORAGE_KEY, token);
    else localStorage.removeItem(TOKEN_STORAGE_KEY);
  },

  clearToken() {
    localStorage.removeItem(TOKEN_STORAGE_KEY);
  },

  getHeaders(customHeaders = {}) {
    const headers = { ...customHeaders };
    const token = this.getToken();
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }
    return headers;
  },

  // =========================================================================
  // Auth API (@embrapa.br OTP)
  // =========================================================================
  async requestOtp(email) {
    const res = await fetch(`${BASE_URL}/auth/request-otp`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ email })
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error?.message || 'Falha ao solicitar código OTP');
    return data;
  },

  async verifyOtp(email, code) {
    const res = await fetch(`${BASE_URL}/auth/verify-otp`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ email, code })
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error?.message || 'Código de verificação inválido');
    if (data.token) {
      this.setToken(data.token);
    }
    return data;
  },

  async getMe() {
    const token = this.getToken();
    if (!token) return { authenticated: false, user: null };
    try {
      const res = await fetch(`${BASE_URL}/auth/me`, {
        headers: this.getHeaders()
      });
      return await res.json();
    } catch {
      return { authenticated: false, user: null };
    }
  },

  async logout() {
    try {
      await fetch(`${BASE_URL}/auth/logout`, {
        method: 'POST',
        headers: this.getHeaders()
      });
    } finally {
      this.clearToken();
    }
  },

  // =========================================================================
  // Analytics & Topic Graph
  // =========================================================================
  async getTopicGraph() {
    const res = await fetch(`${BASE_URL}/analytics/topic-graph`, {
      headers: this.getHeaders()
    });
    if (!res.ok) throw new Error('Falha ao carregar grafo de temas');
    return res.json();
  },

  // =========================================================================
  // Document Operations
  // =========================================================================
  async getHealth() {
    const res = await fetch(`${BASE_URL}/health`, { headers: this.getHeaders() });
    return res.json();
  },

  async getSystemInfo() {
    const res = await fetch(`${BASE_URL}/system`, { headers: this.getHeaders() });
    return res.json();
  },

  async listDocuments() {
    const res = await fetch(`${BASE_URL}/documents`, { headers: this.getHeaders() });
    return res.json();
  },

  async getDocument(id) {
    const res = await fetch(`${BASE_URL}/documents/${encodeURIComponent(id)}`, { headers: this.getHeaders() });
    if (!res.ok) throw new Error(`Document ${id} not found`);
    return res.json();
  },

  async listProjects() {
    const res = await fetch(`${BASE_URL}/projects`, { headers: this.getHeaders() });
    return res.json();
  },

  async listRelations() {
    const res = await fetch(`${BASE_URL}/relations`, { headers: this.getHeaders() });
    return res.json();
  },

  async listSchemas() {
    const res = await fetch(`${BASE_URL}/schemas`, { headers: this.getHeaders() });
    return res.json();
  },

  async getRecentEvents() {
    const res = await fetch(`${BASE_URL}/events`, { headers: this.getHeaders() });
    return res.json();
  },

  async search(query, mode = 'auto') {
    const res = await fetch(`${BASE_URL}/search`, {
      method: 'POST',
      headers: this.getHeaders({ 'Content-Type': 'application/json' }),
      body: JSON.stringify({ query, mode })
    });
    return res.json();
  },

  async updateDocument(id, data) {
    const res = await fetch(`${BASE_URL}/documents/${encodeURIComponent(id)}`, {
      method: 'PUT',
      headers: this.getHeaders({ 'Content-Type': 'application/json' }),
      body: JSON.stringify(data)
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao atualizar documento');
    }
    return res.json();
  },

  async deleteDocument(id) {
    const res = await fetch(`${BASE_URL}/documents/${encodeURIComponent(id)}`, {
      method: 'DELETE',
      headers: this.getHeaders()
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao excluir documento');
    }
    return res.json();
  },

  async createProject(data) {
    const res = await fetch(`${BASE_URL}/projects`, {
      method: 'POST',
      headers: this.getHeaders({ 'Content-Type': 'application/json' }),
      body: JSON.stringify(data)
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao criar projeto');
    }
    return res.json();
  },

  async updateProject(id, data) {
    const res = await fetch(`${BASE_URL}/projects/${encodeURIComponent(id)}`, {
      method: 'PUT',
      headers: this.getHeaders({ 'Content-Type': 'application/json' }),
      body: JSON.stringify(data)
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao atualizar projeto');
    }
    return res.json();
  },

  async deleteProject(id) {
    const res = await fetch(`${BASE_URL}/projects/${encodeURIComponent(id)}`, {
      method: 'DELETE',
      headers: this.getHeaders()
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao excluir projeto');
    }
    return res.json();
  },

  async createRelation(data) {
    const res = await fetch(`${BASE_URL}/relations`, {
      method: 'POST',
      headers: this.getHeaders({ 'Content-Type': 'application/json' }),
      body: JSON.stringify(data)
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao criar relação');
    }
    return res.json();
  },

  async updateRelation(id, data) {
    const res = await fetch(`${BASE_URL}/relations/${encodeURIComponent(id)}`, {
      method: 'PUT',
      headers: this.getHeaders({ 'Content-Type': 'application/json' }),
      body: JSON.stringify(data)
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao atualizar relação');
    }
    return res.json();
  },

  async deleteRelation(id) {
    const res = await fetch(`${BASE_URL}/relations/${encodeURIComponent(id)}`, {
      method: 'DELETE',
      headers: this.getHeaders()
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao excluir relação');
    }
    return res.json();
  },

  async deleteMetadataEnvelope(id) {
    const res = await fetch(`${BASE_URL}/metadata/${encodeURIComponent(id)}`, {
      method: 'DELETE',
      headers: this.getHeaders()
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha ao excluir envelope de metadados');
    }
    return res.json();
  },

  async deepen(id) {
    const res = await fetch(`${BASE_URL}/documents/${encodeURIComponent(id)}/deepen`, {
      method: 'POST',
      headers: this.getHeaders()
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
      headers: this.getHeaders(),
      body: formData
    });
    if (!res.ok) {
      const err = await res.json();
      throw new Error(err.error?.message || 'Falha na ingestão');
    }
    return res.json();
  }
};
