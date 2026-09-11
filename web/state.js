/**
 * ContextLab UI State Management
 */

class AppState {
  constructor() {
    this.currentView = 'inicio'; // 'inicio', 'corpus', 'search', 'projects', 'relations', 'events', 'schemas', 'inspector'
    this.selectedDocId = null;
    this.currentUser = null;
    this.topicGraphData = null;
    this.selectedTopic = null;
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

  get() {
    return this;
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

  setUser(user) {
    this.currentUser = user;
    this.notify();
  }

  setTopicGraphData(data) {
    this.topicGraphData = data;
    this.notify();
  }

  setSelectedTopic(topic) {
    this.selectedTopic = topic;
    this.notify();
  }

  setView(view, docId = null) {
    this.currentView = view;
    if (docId !== null) {
      this.selectedDocId = docId;
    }
    this.notify();
  }

  setData({ documents, projects, relations, schemas, events, systemInfo, currentUser, topicGraphData }) {
    if (documents !== undefined) this.documents = documents;
    if (projects !== undefined) this.projects = projects;
    if (relations !== undefined) this.relations = relations;
    if (schemas !== undefined) this.schemas = schemas;
    if (events !== undefined) this.events = events;
    if (systemInfo !== undefined) this.systemInfo = systemInfo;
    if (currentUser !== undefined) this.currentUser = currentUser;
    if (topicGraphData !== undefined) this.topicGraphData = topicGraphData;
    this.notify();
  }

  setSearchResults(results) {
    this.searchResults = results;
    this.notify();
  }
}

export const state = new AppState();
