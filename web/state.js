/**
 * ContextLab UI State Management
 */

class AppState {
  constructor() {
    this.currentView = 'corpus'; // 'corpus', 'relations', 'schemas', 'events', 'inspector'
    this.selectedDocId = null;
    this.documents = [];
    this.projects = [];
    this.relations = [];
    this.schemas = [];
    this.events = [];
    this.systemInfo = null;
    this.searchQuery = '';
    this.searchMode = 'auto';
    this.searchResults = null;
    this.listeners = [];
  }

  subscribe(listener) {
    this.listeners.push(listener);
    return () => {
      this.listeners = this.listeners.filter(l => l !== listener);
    };
  }

  notify() {
    for (const listener of this.listeners) {
      listener(this);
    }
  }

  setView(view, docId = null) {
    this.currentView = view;
    if (docId !== null) {
      this.selectedDocId = docId;
    }
    this.notify();
  }

  setData({ documents, projects, relations, schemas, events, systemInfo }) {
    if (documents) this.documents = documents;
    if (projects) this.projects = projects;
    if (relations) this.relations = relations;
    if (schemas) this.schemas = schemas;
    if (events) this.events = events;
    if (systemInfo) this.systemInfo = systemInfo;
    this.notify();
  }

  setSearchResults(results) {
    this.searchResults = results;
    this.notify();
  }
}

export const state = new AppState();
